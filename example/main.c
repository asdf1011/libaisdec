
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ais/message.h"
#include "aivdm/packet.h"
#include "aivdm/payload.h"

enum PrintOption {
    PACKET_XML,
    MESSAGE_XML,
    NONE
};

static void usage(char* program) {
    printf("Usage: %s [options] [filename] ...\n", program);
    printf("Decode aivdm files to xml.\n");
    printf("If no filenames are given, it will decode from stdin.\n");
    printf("Options:\n");
    printf("   -h    Display this help.\n");
    printf("   -m    Print the message xml (default).\n");
    printf("   -p    Print the packet xml.\n");
    printf("   -q    Quiet output (don't print xml).\n");
}

int encodeBinaryPayload(const struct Payload0* input, struct EncodedData* output) {
    struct Payload payload;
    payload.items = malloc(sizeof(int) * input->count);
    payload.count = input->count;
    int i;
    for (i = 0; i < input->count; ++i) {
        if (input->items[i].character.length == 1) {
            payload.items[i] = input->items[i].character.buffer[0];
        }
    }
    if (!encodePayload(output, &payload)) {
        fprintf(stderr, "Failed to get binary payload!\n");
        free(payload.items);
        return 0;
    }
    free(payload.items);
    return 1;
}

/**
 * Update the binary payload given the new packet.
 *
 * Returns 1 if the binary payload is complete, otherwise 0.
 */
int updatePayload(int *fragmentNumber, int *fragmentCount,
        const struct Packet *packet, struct EncodedData *payloadBinary) {
    if ((*fragmentCount != 0 && *fragmentCount != packet->fragmentCount) ||
            *fragmentNumber + 1 != packet->fragmentNumber) {
        fprintf(stderr, "Was expecting fragment %i / %i, got %i / %i! "
                "Skipping packet.\n", *fragmentNumber + 1, *fragmentCount,
                packet->fragmentNumber, packet->fragmentCount);
        *fragmentNumber = 0;
        *fragmentCount = 0;
        return 0;
    }
    *fragmentNumber = packet->fragmentNumber;
    *fragmentCount = packet->fragmentCount;
    if (encodeBinaryPayload(&packet->payload, payloadBinary)) {
        if (*fragmentNumber == *fragmentCount) {
            // This fragment is complete.
            *fragmentNumber = 0;
            *fragmentCount = 0;
            return 1;
        }
    }
    else {
        fprintf(stderr, "Failed to decode packet payload! Skipping packet.\n");
        *fragmentNumber = 0;
        *fragmentCount = 0;
    }
    return 0;
}

int decode(FILE* input, enum PrintOption printing) {
    char data[1024];
    int dataLength = 0;
    struct EncodedData payloadBinary = {0};
    int fragmentCount = 0, fragmentNumber = 0;
    for (;;) {
        dataLength += fread(data, 1, sizeof(data) - dataLength, input);
        if (dataLength == 0) {
            break;
        }

        /* Decode the aivdm */
        BitBuffer buffer = {(unsigned char*)data, 0, dataLength * 8};
        struct Packet packet;
        if (decodePacket(&buffer, &packet)) {
            /* Print the decoded data */
            if (printing == PACKET_XML) {
                printXmlPacket(&packet, 0, "packet");
            }
            if (updatePayload(&fragmentNumber, &fragmentCount, &packet,
                        &payloadBinary)) {
                BitBuffer messageBuffer = {
                    (unsigned char*)payloadBinary.buffer, 0,
                    payloadBinary.num_bits - packet.numFillBits};
                payloadBinary.num_bits = 0;

                struct Message message;
                if (decodeMessage(&messageBuffer, &message)) {
                    if (printing == MESSAGE_XML) {
                        printXmlMessage(&message, 0, "message");
                    }
                    freeMessage(&message);
                }
                else {
                    fprintf(stderr, "Failed to decode payload message! Skipping packet.\n");
                }
            }

            freePacket(&packet);
        }
        else {
            fprintf(stderr, "Failed to decode packet! Skipping.\n");
            char* next = memchr(data, '\n', dataLength);
            if (next == 0) {
                /* No more packets. */
                break;
            }
            buffer.buffer = (unsigned char*)next + 1;
        }

        /* Move the data still to decode to the start of the buffer. */
        dataLength -= (char*)buffer.buffer - data;
        memcpy(data, buffer.buffer, dataLength);
    }
    free(payloadBinary.buffer);
    return 1;
}

int main(int argc, char* argv[]) {
    /**
     * We parse options by hand to avoid a dependancy on getopt.
     */
    int i;
    enum PrintOption printing = MESSAGE_XML;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'h':
                usage(argv[0]);
                return EXIT_SUCCESS;
            case 'm':
                printing = MESSAGE_XML;
                break;
            case 'p':
                printing = PACKET_XML;
                break;
            case 'q':
                printing = NONE;
                break;
            default:
                fprintf(stderr, "Unknown option '%s'! See %s -h for more details.\n",
                        argv[i], argv[0]);
                return EXIT_FAILURE;
            }
        }
        else {
            /* We've found the first argument */
            break;
        }
    }

    if (i == argc) {
        fprintf(stderr, "Decoding from stdin...\n");
        if (!decode(stdin, printing)) {
            return EXIT_FAILURE;
        }
    }
    else {
        for (; i < argc; ++i) {
            FILE* datafile = fopen(argv[i], "rb");
            if (datafile == 0) {
                fprintf(stderr, "Failed to open '%s': %s\n", argv[i], strerror(errno));
                return EXIT_FAILURE;
            }
            if (!decode(datafile, printing)) {
                fclose(datafile);
                return EXIT_FAILURE;
            }
            fclose(datafile);
        }
    }

    return EXIT_SUCCESS;
}

