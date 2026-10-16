/*
    ForthHostFileSystem.h
    R.A.Lincoln   October 2026

		File IO Handler
		Called by the IO dispatcher to implement

*/

#pragma once
#include <fstream>

#define fsOpen				0x01
#define fsClose				0x02
#define fsRead				0x03
#define fsWrite				0x04
#define fsSeek				0x05

#define fsReadMode		0x10
#define	fsWriteMode		0x20
#define	fsRwMode			0x30
#define fsAppendMode	0x40


class HostFileSystemClass {

private:

	std::array<std::unique_ptr<std::fstream>, 10> files;		// Fixed size open file table, slot 0 not used

	uint8_t		cmdReg;
	uint8_t		modeReg;

	uint16_t	idReg;
	char* 		nameReg;
	char* 		bufferReg;
	uint16_t 	sizeReg;

	uint16_t	statusReg;

public:


	//	Internal register access
	//
	void setCmd(uint16_t cmd) {
		cmdReg = cmd & 0x0f;
		modeReg = cmd & 0xf0;
		run();																		// Setting the command runs it
	}

	void setID(uint16_t id) {
		idReg = id;
	}

	void setName(char* name) {
		nameReg = name;
	}

	void setBuffer(char* buffer) {
		bufferReg = buffer;
	}

	void setSize(uint16_t size) {
		sizeReg = size;
	}

	uint16_t getID() {
		return idReg;
	}

	uint16_t getStatus() {
		return statusReg;
	}


	//	Run the command
	//
	void run() {
		switch(cmdReg) {
		case fsOpen:
			idReg = open(nameReg, modeReg);
			statusReg = idReg;
			break;

		case fsClose:
			statusReg = close(idReg);
			break;

		case fsRead:
			statusReg = read(idReg, bufferReg, sizeReg);
			break;

		case fsWrite:
			statusReg = write(idReg, bufferReg, sizeReg);
			break;

		case fsSeek:
			statusReg = seek(idReg, sizeReg, 0);
			break;

		default:
			break;
		}
	}


private:


	//	Open
	//
	uint16_t open(char* name, uint8_t mode) {
  	std::ios::openmode ios_mode = std::ios::in;				// interpret file mode
    if (mode & fsWriteMode) ios_mode |= std::ios::out;
    if (mode & fsAppendMode) ios_mode |= std::ios::app;

    auto fs = std::make_unique<std::fstream>(name, ios_mode);
    if (!fs->is_open()) return 0;											// -->

    for (size_t i = 1; i < files.size(); ++i) {				// Find an existing empty slot, start at 1
    	if (!files[i] || !files[i]->is_open()) {
    		files[i] = std::move(fs);
      	return i;																			// -->
      }
    }

		fs->close();																			// Fall through, no slot - close it
    return 0;
  }


	//	Close
	//
  uint16_t close(uint16_t fid) {
  	if (fid < 0 || fid >= files.size() || !files[fid] || !files[fid]->is_open()) {
    	return 0;																				// -->
    }

    files[fid]->close();
    files[fid].reset();

		return fid;
  }


	//	Read
	//
  int read(uint16_t fid, char* buffer, uint16_t size) {
  	if (fid < 0 || fid >= files.size() || !files[fid] || !files[fid]->is_open()) {
    	return 0;																				// -->
    }

    files[fid]->read(buffer, size);

		int bytesRead = files[fid]->gcount();
		if (bytesRead >= 0 && bytesRead < size) {					// Trailing null if the buffer isn't full
    	buffer[bytesRead] = '\0';
		}

    return bytesRead;
  }


	//	Write
	//
	int write(uint16_t fid, const char* buffer, uint16_t size) {
    if (fid >= files.size() || !files[fid] || !files[fid]->is_open()) {
        return 0;																			// -->
    }

		// std::cout << "fs write 2 - size: " << (int)size << " buffer ptr: " << (void*)buffer << "\n";
    files[fid]->write(buffer, size);
    files[fid]->flush();

    if (!*files[fid]) {			    											// Check if the write operation failed
        return 0; 																		// -->
    }
    return static_cast<int>(size); 										//	bytes written on success
	}


	//	Seek
	//
	int seek(uint16_t fid, int32_t offset, int whence) {
    if (fid >= files.size() || !files[fid] || !files[fid]->is_open()) {
        return 0;
    }

    std::ios::seekdir dir;
    switch (whence) {
        case 0: dir = std::ios::beg; break; // SEEK_SET
        case 1: dir = std::ios::cur; break; // SEEK_CUR
        case 2: dir = std::ios::end; break; // SEEK_END
        default: return 0;														// -->
    }

    files[fid]->clear();															// Clear EOF flags that can sometimes block a successful seek
    files[fid]->seekg(offset, dir);										// Read pointer
    files[fid]->seekp(offset, dir);										// Write pointer

    if (!*files[fid]) {
        return 0; 																		// Seek failed (e.g., out of bounds)
    }
    return static_cast<int>(files[fid]->tellg());			//	Return current absolute position, or 0 for success

	}

};


