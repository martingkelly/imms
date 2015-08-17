# Porting IMMS #

So you want to get IMMS running on a new OS, device, or other platform? Great! Here are some requirements you will need to keep in mind.

  1. **Does immsd compile for your OS?** See requirements in the [INSTALL](http://code.google.com/p/imms/source/browse/trunk/imms/INSTALL) file.
  1. **Can your development language talk to unix-style file sockets?** immsd uses a file socket (usually created as ~/.immsd/socket) to send and receive commands. If you are porting IMMS or writing a plugin it will probably need to use this. (as an advanced alternative you could try using the Imms object (immscore/imms.h) directly, pumping events and injecting the playlist externally)