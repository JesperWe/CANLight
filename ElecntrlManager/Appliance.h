/*
 * Appliance.h
 *
 *  Created on: 20 dec 2009
 *      Author: sysadm
 */

#ifndef APPLIANCE_H_
#define APPLIANCE_H_

#include <QString>

namespace ECS {

class Appliance {

public:

	Appliance();
	Appliance( QString myId, QString myDescr );
	virtual ~Appliance();

 	QString id;
	QString description;

	enum Types {
        LEDLight,
        Switch,
        Actuators
    };
    Q_ENUMS( Types );

};

}

#endif /* APPLIANCE_H_ */
