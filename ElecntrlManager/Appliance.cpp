/*
 * Appliance.cpp
 *
 *  Created on: 20 dec 2009
 *      Author: sysadm
 */

#include "Appliance.h"

namespace ECS {

	Appliance::Appliance() {
	}

	Appliance::Appliance( QString myId, QString myDescr ) {
		id = myId;
		description = descr;
	}

	Appliance::~Appliance() {
	}

}
