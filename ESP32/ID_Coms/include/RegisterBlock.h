/*
 * BasePipe.h
 *
 *  Created on: 9 Jan 2019
 *      Author: xasin
 */

#ifndef XASLIBS_ID_COMS_BASEPIPE_H_
#define XASLIBS_ID_COMS_BASEPIPE_H_

#include <cstring>
#include <functional>
#include <vector>
#include <map>

namespace Xasin {
namespace Communication {

// The virtual base class for Xasin-communication.
// It represents the endpoint of a connection between two devices.
// Communication itself is done by sending ID'd packets to "registers" in the other device.
// Additionally, read requests can be issued to fetch a given register from the other device.

// This way, this kind of communication can be used both for Master<->Master (both sending packets),
// And Master<->Slave (Master writing packets and issuing read requests)

// Thusly, each register must contain a callback function upon data receival,
// As well as a data pointer to be read from (optional)

struct Data_Packet {
	size_t length;
	const void * data;
};

class RegisterBlock;

class SlaveChannel {
private:
	RegisterBlock &mainRegister;

public:
	SlaveChannel(RegisterBlock &mainRegister);

	virtual void send_update(uint16_t ID, const Data_Packet data);
};

class ComRegister {
private:
	RegisterBlock &registerBlock;

protected:
	friend RegisterBlock;

	void 	*dataLocation;
	size_t 	dataLength;

	std::function<void (const Data_Packet)> write_cb;

public:
	const uint16_t ID;
	const bool write_allowed;

	ComRegister(uint16_t ID, RegisterBlock &regBlock, Data_Packet dataLoc = {0, nullptr}, bool write_allowed = false);

	void update(const Data_Packet data);
	void update();
	void update(const std::string &data);

	template<class T>
	void update(const T &data) {
		update({sizeof(T), reinterpret_cast<void*>(&data)});
	};
};

class RegisterBlock {
protected:
	friend SlaveChannel;
	std::vector<SlaveChannel *> channels;
	std::map<uint16_t, ComRegister  *> comRegisters;

	void write_register(uint16_t ID, const Data_Packet data);
	const Data_Packet read_register(uint16_t ID);

public:
	RegisterBlock();

	void send_update(uint16_t ID, const Data_Packet data);
};

} /* namespace Communication */
} /* namespace Xasin */

#endif /* XASLIBS_ID_COMS_BASEPIPE_H_ */
