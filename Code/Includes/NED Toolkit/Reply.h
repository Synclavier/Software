/*	NED Toolkit - SYNCnet Protocol Reply Class		Copyright � 1991 by New England Digital Corp.  All rights reserved.		This file defines objects & methods for parsing packets received over the	SYNCnet protocol.*/#ifndef NED__REPLY#define NED__REPLY#ifndef NED__SYNCNET#include	"SYNCnet.h"#endif#ifndef NED__REQUEST#include	"Request.h"#endif// this provides simple set of methods for streaming data fields out of a packet// load the packet with Set(), call a series of Get()// this extends the message get_� routinesclass Reply  {public:	// constructors	Reply()								{ Clear(); }	Reply(PACKET *pkt)					{ Set(pkt); }	void			Set(PACKET *pkt);	// attach a C PACKET struct	void			Clear();			// no such		int				Get(void *data);	// get the whole thing (return size)	int 			Get(void *data,int size);	// get a struct (return size)	int8			Getint8();			// byte	int16			Getint16();			// shortword	int32			Getint32();			// longword	void			Getset(SET *a);		// set	void			Getbits(int32 bits[],int count);	// get (count) bits	char*			Getstring();		// return C string (must be copied)	void*			Getref(int size = 0);	// return address of struct (must be copied)	PACKET*			Getpacket()			{ return rpkt; }	// return base packet	int				Getleft()			{ return stop-scan; }protected:	PACKET			*rpkt;				// reply packet	char			*scan;				// packet scan	char			*stop;				// end of packet	};// this class provides a funky form of virtual reply// it is set by passing a VReply::Reset() to Syncnet req_� calls// calls to Get�() cause busy waits (Sync()) until the reply has arrived// use this for simple, synchronous, request/reply interactionsclass VReply : private Reply {public:	// constructor & destructor	VReply();	~VReply();		// extra methods	Syncnet_Error	Err()			{ Sync(); return error; }		// what happened to the answer	Request			*Reset();										// clear the request	Reply&			Sync();			// force arrival	void			Wait();			// busy wait without events	// cast	operator Request*()				{ return Reset(); }				// make a request to pass syncnet		int				Get(void *data)	{ return Sync().Get(data); }	// get the whole thing (return size)	int 			Get(void *data,int size) { return Sync().Get(data,size); }	// get a struct (return size)	int8			Getint8()		{ return Sync().Getint8(); }	// byte	int16			Getint16()		{ return Sync().Getint16(); }	// shortword	int32			Getint32()		{ return Sync().Getint32(); }	// longword	void			Getset(SET *a)	{ Sync().Getset(a); }			// set	void			Getbits(int32 bits[],int count)									{ Sync().Getbits(bits,count); }	// get (count) bits	char*			Getstring()		{ return Sync().Getstring(); }		// return C string (must be copied)	void*			Getref(int size = 0)	{ return Sync().Getref(size); }	// return address of struct (must be copied)		PACKET			*Getpacket()	{ return Sync().Getpacket(); }	// return base packet	int				Getleft()		{ return Sync().Getleft(); }	// return size remaining	protected:		Syncnet_Error	error;				// Syncnet error status	Request			R;					// used to await packet	};	#endif	// NED__REPLY