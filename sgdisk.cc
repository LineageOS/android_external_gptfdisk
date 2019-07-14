// sgdisk.cc
// Command-line-based version of gdisk. This program is named after sfdisk,
// and it can serve a similar role (easily scripted, etc.), but it's used
// strictly via command-line arguments, and it doesn't bear much resemblance
// to sfdisk in actual use.
//
// by Rod Smith, project began February 2009; sgdisk begun January 2010.

/* This program is copyright (c) 2009-2011 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <errno.h>
#include "gptcl.h"
#include <fcntl.h>
#include <unistd.h>

#include "sgdisk.h"

using namespace std;

#define MAX_OPTIONS 50

int sgdisk_read(const char* device, sgdisk_partition_table& ptbl,
                vector<sgdisk_partition*>& partitions) {
    BasicMBRData mbrData;
    GPTData gptData;
    GPTPart partData;
    int numParts = 0;

    /* Silence noisy underlying library */
    int stdout_fd = dup(STDOUT_FILENO);
    int stderr_fd = dup(STDERR_FILENO);
    int silence = open("/dev/null", 0);
    dup2(silence, STDOUT_FILENO);
    dup2(silence, STDERR_FILENO);

    if (!mbrData.ReadMBRData((string) device)) {
        cerr << "Failed to read MBR" << endl;
        return 8;
    }

    switch (mbrData.GetValidity()) {
    case mbr:
        ptbl.type = MBR;
        ptbl.guid.clear();
        for (size_t i = 0; i < MAX_MBR_PARTS; i++) {
            if (mbrData.GetLength(i) > 0) {
                char typebuf[2+8+1];
                sprintf(typebuf, "%x", (unsigned int)mbrData.GetType(i));
                sgdisk_partition* part = new sgdisk_partition;
                part->num = i + 1;
                part->type = typebuf;
                partitions.push_back(part);
            }
        }
        break;
    case gpt:
        gptData.JustLooking();
        if (!gptData.LoadPartitions((string) device)) {
            cerr << "Failed to read GPT" << endl;
            return 9;
        }

        ptbl.type = GPT;
        ptbl.guid = gptData.GetDiskGUID().AsString();
        for (size_t i = 0; i < gptData.GetNumParts(); i++) {
            partData = gptData[i];
            if (partData.GetFirstLBA() > 0) {
                sgdisk_partition* part = new sgdisk_partition;
                part->num = i + 1;
                part->type = partData.GetType().AsString();
                part->guid = partData.GetUniqueGUID().AsString();
                part->name = partData.GetDescription();
                partitions.push_back(part);
            }
        }
        break;
    default:
        cerr << "Unknown partition table" << endl;
        return 10;
    }

    fflush(stdout);
    fflush(stderr);
    dup2(stdout_fd, STDOUT_FILENO);
    dup2(stderr_fd, STDERR_FILENO);
    close(silence);

    return 0;
}

extern "C" int main(int argc, char *argv[]) {
    return 0;
}
