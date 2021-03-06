/*
  BOLIDE_Player.cpp - Modified for XYZrobot ATmega 1280 control board.
  Copyright (c) 2015 Wei-Shun You. XYZprinting Inc.  All right reserved.
  BOLIDE_Player.cpp
*/
/*
  BioloidController.cpp - ArbotiX Library for Bioloid Pose Engine
  Copyright (c) 2008-2012 Michael E. Ferguson.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "BOLIDE_Player.h"
#include "SerialProtocol.h"
#include "ProgramMemoryProtocol.h"

static char packet_send[110];
static unsigned int checksum_1;
static unsigned int checksum_2;

#include <serstream>
#include <new>

#define SERVO_TO_POSE(x) (uint16_t)((x) << A1_16_SHIFT)
#define POSE_TO_SERVO(x) (uint16_t)((x) >> A1_16_SHIFT)

namespace std
{
    ohserialstream cout(Serial);
}

/* new-style setup */
void BOLIDE_Player::setup(unsigned long baud, uint8_t servo_cnt, ProgramMemoryProtocol& programMemory, SerialProtocol& serialChannel)
{
    A1_16_Ini(baud, serialChannel);
    _serialChannel = &serialChannel;
    _programMemory = &programMemory;

    poseSize = servo_cnt;

    // setup storage
    id_ = new uint8_t[servo_cnt];
    pose_ = new uint16_t[servo_cnt];
    nextpose_ = new uint16_t[servo_cnt];
    speed_ = new int[servo_cnt];

    // initialize
    for (uint8_t i = 0; i < poseSize; i++)
    {
        id_[i] = i + 1;
        pose_[i] = SERVO_TO_POSE(512);
        nextpose_[i] = SERVO_TO_POSE(512);
    }
    interpolating = 0;
    playing = 0;
    lastframe_ = millis();
}

void BOLIDE_Player::setId(uint8_t index, uint8_t id)
{
    id_[index] = id;
}

uint8_t BOLIDE_Player::getId(uint8_t index)
{
    return id_[index];
}

/* load a named pose from FLASH into nextpose. */
void BOLIDE_Player::loadPose(const unsigned int *addr)
{
    poseSize = _programMemory->readWordNear(addr); // number of servos in this pose

    enum { poseDataOffset = 1 };
    for (uint8_t servoIndex = 0; servoIndex < poseSize; servoIndex++)
    {
        nextpose_[servoIndex] = SERVO_TO_POSE(_programMemory->readWordNear(addr + servoIndex + poseDataOffset));
    }
}

/* read in current servo positions to the pose. */
void BOLIDE_Player::readPose()
{
    readPoseTo(pose_);
}

void BOLIDE_Player::readPoseTo(uint16_t *saveToPose)
{
    readPoseTo(saveToPose, RAM_Joint_Position);
}

void BOLIDE_Player::readPosGoalTo(uint16_t* saveToPose)
{
    readPoseTo(saveToPose, RAM_Position_Goal);
}

void BOLIDE_Player::readPoseTo(uint16_t *saveToPose, unsigned char addr)
{
    for (int i = 0; i < poseSize; i++)
    {
        saveToPose[i] = SERVO_TO_POSE(ReadDataRAM2(id_[i], addr));

        delay(25);
    }
}

/* write pose out to servos using sync write. */
void BOLIDE_Player::writePose()
{
    if (traceSeqPlay_)
    {
        printPose(pose_, "trace");

//        uint16_t positionGoal[poseSize];
//        readPosGoalTo(positionGoal);
//        printPose(positionGoal, "goal");
    }

    packet_send[0] = (char)0xff;
    packet_send[1] = (char)0xff;
    packet_send[2] = (char)(7 + 5 * poseSize);
    packet_send[3] = (char)0xfe;
    packet_send[4] = CMD_I_JOG;
    // 5-6 - checksum
    checksum_1 = (unsigned int)(packet_send[2] ^ packet_send[3] ^ packet_send[4]);

    const int commandOffset = 7;
    for (int i = 0; i < poseSize; i++)
    {
        int servoPosition = POSE_TO_SERVO(pose_[i]);
        const int servoOffset = 5 * i;

        packet_send[7 + servoOffset] = LO_BYTE(servoPosition);
        checksum_1 ^= packet_send[7 + servoOffset];

        packet_send[8 + servoOffset] = HI_BYTE(servoPosition);
        checksum_1 ^= packet_send[8 + servoOffset];

        packet_send[9 + servoOffset] = recoveringTorque_ ? 3 : 0; // 0 (position control) / 1 (speed control) / 2 (torque off) /3 (position control servo on)
        checksum_1 ^= packet_send[9 + servoOffset];

        packet_send[10 + servoOffset] = id_[i];
        checksum_1 ^= packet_send[10 + servoOffset];

        packet_send[11 + servoOffset] = recoveringTorque_ ? 0 : 2;  // playtime, unit 10ms
        checksum_1 ^= packet_send[11 + servoOffset];
    }
    checksum_1 &= 0xfe;
    packet_send[5] = (char)checksum_1;
    checksum_2 = (~checksum_1) & 0xfe;
    packet_send[6] = (char)checksum_2;
    for (int byteIndex = 0; byteIndex < packet_send[2]; byteIndex++)
    {
        _serialChannel->write(packet_send[byteIndex]);
    }

    recoveringTorque_ = false;
}

/* set up for an interpolation from pose to nextpose over TIME 
    milliseconds by setting servo speeds. */
void BOLIDE_Player::interpolateSetup(unsigned int time)
{
    unsigned int frames = (time / A1_16_FRAME_LENGTH) + 1;
    total_frame = frames;                //Wei-Shun You edits: record the frames between poses
    lastframe_ = millis();
    // set speed each servo...
    for (uint8_t i = 0; i < poseSize; i++)
    {
        if (nextpose_[i] > pose_[i])
        {
            speed_[i] = (nextpose_[i] - pose_[i]) / frames + 1;
        }
        else
        {
            speed_[i] = (pose_[i] - nextpose_[i]) / frames + 1;
        }
    }
    interpolating = 1;
}

/* interpolate our pose, this should be called at about 30Hz. */
void BOLIDE_Player::interpolateStep()
{
    if (interpolating == 0)
    {
        return;
    }

    int complete = poseSize;
    while (millis() - lastframe_ < A1_16_FRAME_LENGTH)
    {
    }
    frame_counter++;
    lastframe_ = millis();
    // update each servo
    for (uint8_t i = 0; i < poseSize; i++)
    {
        int diff = nextpose_[i] - pose_[i];
        if (diff == 0)
        {
            complete--;
        }
        else
        {
            if (diff > 0)
            {
                if (diff < speed_[i])
                {
                    pose_[i] = nextpose_[i];
                    complete--;
                }
                else
                {
                    pose_[i] += speed_[i];
                }
            }
            else
            {
                if ((-diff) < speed_[i])
                {
                    pose_[i] = nextpose_[i];
                    complete--;
                }
                else
                {
                    pose_[i] -= speed_[i];
                }
            }
        }
    }
    if ((complete <= 0) && (frame_counter >= total_frame))
    {
        interpolating = 0;
        frame_counter = 0;
    }
    writePose();
}

/* get a servo value in the current pose */
int BOLIDE_Player::getCurPose(int id)
{
    for (int i = 0; i < poseSize; i++)
    {
        if (id_[i] == id)
        {
            return POSE_TO_SERVO(pose_[i]);
        }
    }
    return -1;
}

/* get a servo value in the next pose */
int BOLIDE_Player::getNextPose(int id)
{
    for (int i = 0; i < poseSize; i++)
    {
        if (id_[i] == id)
        {
            return POSE_TO_SERVO(nextpose_[i]);
        }
    }
    return -1;
}

/* set a servo value in the next pose */
void BOLIDE_Player::setNextPose(int id, int pos)
{
    for (int i = 0; i < poseSize; i++)
    {
        if (id_[i] == id)
        {
            nextpose_[i] = SERVO_TO_POSE(pos);
            return;
        }
    }
}

void BOLIDE_Player::printPose(uint16_t *poseToPrint, const char* label)
{
//    std::cout << std::endl;
    for (int i = 0; i < poseSize; i++)
    {
        if (i > 0)
        {
            std::cout << ", ";
        }
        std::cout << POSE_TO_SERVO(poseToPrint[i]);
    }
    std::cout << "  <<<<< " << label << std::endl;
}

/* play a sequence. */
void BOLIDE_Player::playSeq(const transition_t *addr)
{
    sequence = (transition_t *)addr;

    TransitionConfig *config = (TransitionConfig *)(void*)addr;
    transitions = _programMemory->readWordNear(&config->totalPoses);

    // load a transition
    const transition_t *firstFrame = ++sequence;
    int time = _programMemory->readWordNear(&firstFrame->time);
    if (torqueOff_)
    {
        torqueOff_ = false;

        readPose();
        traceSeqPlay_ = true;
        recoveringTorque_ = true;

        time = std::max(time, 400);
    }


    loadPose((const unsigned int *)_programMemory->readWordNear(&firstFrame->pose));
    interpolateSetup(time);
    transitions--;
    playing = 1;
}

/* keep playing our sequence */
void BOLIDE_Player::play()
{
    if (playing == 0)
    {
        return;
    }
    if (interpolating > 0)
    {
        interpolateStep();
    }
    else
    {  // move onto next pose
        sequence++;
        if (transitions > 0)
        {
            loadPose((const unsigned int *)_programMemory->readWordNear(&sequence->pose));
            interpolateSetup(_programMemory->readWordNear(&sequence->time));
            transitions--;
        }
        else
        {
            playing = 0;
            traceSeqPlay_ = false;
        }
    }
}

void BOLIDE_Player::torqueOff()
{
    A1_16_TorqueOff(A1_16_Broadcast_ID);

    torqueOff_ = true;
}

void BOLIDE_Player::printPose()
{
    uint16_t currentPose[poseSize];

    readPoseTo(currentPose);
    printPose(currentPose, "current pose");

    readPosGoalTo(currentPose);
    printPose(currentPose, "current goal");
}





