#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "jvs.h"

int main(int argc, char **argv)
{

	JVSPacket packet;
	packet.destination = 0;
	packet.length = 0;

	JVSStatus status = JVS_STATUS_ERROR;

	assert(status == JVS_STATUS_SUCCESS);

	return EXIT_SUCCESS;
}
