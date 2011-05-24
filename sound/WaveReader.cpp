
#include "precompiled.h"

// Copyright 2002-2004 Frozenbyte Ltd.

#include "WaveReader.h"
#include "AmplitudeArray.h"
#include "../system/Logger.h"
#include "../util/assert.h"
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include "../filesystem/file_package_manager.h"
#include "../filesystem/input_stream.h"


#ifdef _WIN32
#include <windows.h>

#else
#include <SDL_sound.h>

#include <boost/scoped_array.hpp>

#include "system/Miscellaneous.h"

#endif  // _WIN32


using namespace boost;
using namespace std;
using namespace frozenbyte;

namespace sfx {
namespace {

	void logParseError(const std::string &file, const std::string &info)
	{
		
#pragma message(" ** Wave reader errors disabled as they seem to be always triggered ** ")

		/*
		string errorString = "Error parsing wave file: ";
		errorString += file;

		Logger::getInstance()->error(errorString.c_str());
		if(!info.empty())
			Logger::getInstance()->info(info.c_str());
		*/
	}


#ifdef _WIN32

	struct CharDeleter
	{
		void operator () (WAVEFORMATEX *ptr)
		{
			if(!ptr)
				return;

			CHAR *charPtr = reinterpret_cast<CHAR *> (ptr);
			delete[] charPtr;
		}
	};


#endif  // _WIN32


} // unnamed


#ifdef _WIN32

struct WaveReader::Data
{
	string fileName;

	shared_ptr<WAVEFORMATEX> waveFormat;
	HMMIO ioHandle;
	MMCKINFO riffChunk;

	boost::shared_array<char> buffer;

	Data()
	:	ioHandle(0)
	{
	}

	~Data()
	{
		free();
	}

	void open(const string &file)
	{
		fileName = file;

		filesystem::InputStream fileStream = filesystem::FilePackageManager::getInstance().getFile(file);
		if(!fileStream.isEof())
		{
			int size = fileStream.getSize();
			buffer.reset(new char[size]);
			fileStream.read(buffer.get(), size);

			MMIOINFO info = { 0 };
			info.pchBuffer = buffer.get();
			info.fccIOProc = FOURCC_MEM;
			info.cchBuffer = size;

			ioHandle = mmioOpen(0, &info, MMIO_READ);
		}

		if(!ioHandle)
		{
			string errorString = "Cannot open wave file: ";
			errorString += file;

			Logger::getInstance()->warning(errorString.c_str());
		}

		/*
		ioHandle = mmioOpen(const_cast<char *> (fileName.c_str()), NULL, MMIO_ALLOCBUF | MMIO_READ );
		if(!ioHandle)
		{
			string errorString = "Cannot open wave file: ";
			errorString += file;

			Logger::getInstance()->warning(errorString.c_str());
		}
		*/
	}

	void findPosition()
	{
		MMCKINFO chunkInfo = { 0 };
		PCMWAVEFORMAT pcmWaveFormat = { 0 };

		if(mmioDescend(ioHandle, &riffChunk, NULL, 0) != MMSYSERR_NOERROR)
		{
			logParseError(fileName, "findPosition::mmioDescend");
			free();

			return;
		}

		if((riffChunk.ckid != FOURCC_RIFF) || (riffChunk.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
		{
			logParseError(fileName, "findPosition::waveChunkCheck");
			free();

			return;
		}

		chunkInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
		if(mmioDescend(ioHandle, &chunkInfo, &riffChunk, MMIO_FINDCHUNK) != MMSYSERR_NOERROR)
		{
			logParseError(fileName, "findPosition::fmtChunkCheck");
			free();

			return;
		}

		if(chunkInfo.cksize < sizeof(PCMWAVEFORMAT))
		{
			logParseError(fileName, "findPosition::chunkInfoSize");
			free();

			return;
		}

		if(mmioRead(ioHandle, (HPSTR) &pcmWaveFormat, sizeof(pcmWaveFormat)) != sizeof(pcmWaveFormat))
		{
			logParseError(fileName, "findPosition::waveFormatSize");
			free();

			return;
		}

		if(pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
		{
			waveFormat.reset(reinterpret_cast<WAVEFORMATEX *> (new CHAR[sizeof(WAVEFORMATEX)]), CharDeleter());
			memcpy(waveFormat.get(), &pcmWaveFormat, sizeof(pcmWaveFormat));
			waveFormat->cbSize = 0;
		}
		else
		{
			WORD extraBytes = 0L;
			if(mmioRead(ioHandle, (CHAR*) &extraBytes, sizeof(WORD)) != sizeof(WORD))
			{
				logParseError(fileName, "findPosition::readExtreByteAmount");
				free();

				return;
			}

			waveFormat.reset(reinterpret_cast<WAVEFORMATEX *> (new CHAR[sizeof(WAVEFORMATEX) + extraBytes]), CharDeleter());
			memcpy(waveFormat.get(), &pcmWaveFormat, sizeof(pcmWaveFormat));
			waveFormat->cbSize = extraBytes;

			if(mmioRead(ioHandle, (CHAR*)(((BYTE*)&(waveFormat->cbSize))+sizeof(WORD)), extraBytes ) != extraBytes)
			{
				logParseError(fileName, "findPosition::readExtreBytes");
				free();

				return;
			}
		}

		if(mmioAscend(ioHandle, &chunkInfo, 0) != MMSYSERR_NOERROR)
		{
			logParseError(fileName, "findPosition::mmioAscend");
			free();

			return;
		}
	}

	void resetRead()
	{
		MMCKINFO chunkInfo = { 0 };
		if(-1 == mmioSeek(ioHandle, riffChunk.dwDataOffset + sizeof(FOURCC), SEEK_SET))
			return;

		// Search the input file for the 'data' chunk.
		chunkInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
		if(0 != mmioDescend(ioHandle, &chunkInfo, &riffChunk, MMIO_FINDCHUNK))
			return;
	}

	void readAmplitudeArray(int tickTime, AmplitudeArray &array)
	{
		FB_ASSERT(tickTime >= 10);
		if(!ioHandle)
			return;

		if(waveFormat->wBitsPerSample != 8 && waveFormat->wBitsPerSample != 16)
		{
			logParseError(fileName, "readAmplitudeArray::unsupported bits per sample amount");
			free();

			return;
		}

		if(waveFormat->nChannels != 1 && waveFormat->nChannels != 2)
		{
			logParseError(fileName, "readAmplitudeArray::unsupported channel amount");
			free();

			return;
		}

		int sampleSize = waveFormat->nSamplesPerSec * waveFormat->nChannels * waveFormat->wBitsPerSample / 8;
		int amplitudeSamplesPerSecond = 1000 / tickTime;
		int amplitudeSampleDelta = sampleSize / amplitudeSamplesPerSecond;
		int amplitudeSampleAmount = riffChunk.cksize / amplitudeSampleDelta + 1;

        MMIOINFO statusInfo = { 0 };
        if(mmioGetInfo(ioHandle, &statusInfo, 0) != MMSYSERR_NOERROR)
		{
			logParseError(fileName, "readAmplitudeArray::mmioGetInfo");
			free();

			return;
		}

		array.setSampleAmount(amplitudeSampleAmount);
		int index = 0;

		unsigned int bytesRead = 0;
		int loopIndex = 0;

		//vector<unsigned char> sampleBuffer(1000);
		vector<unsigned char> sampleBuffer(amplitudeSampleDelta / 2);
		int sampleIndex = 0;

		while(bytesRead < riffChunk.cksize)
		{
			++loopIndex;
            if(statusInfo.pchNext == statusInfo.pchEndRead)
            {
                if(mmioAdvance(ioHandle, &statusInfo, MMIO_READ) != MMSYSERR_NOERROR)
				{
					logParseError(fileName, "readAmplitudeArray::mmioAdvance");
					free();

					return;
				}

                if(statusInfo.pchNext == statusInfo.pchEndRead)
				{
					//logParseError(fileName, "readAmplitudeArray::unexpected end");
					free();

					return;
				}
            }

			int amplitude = 0;
			//for(int i = 0; i < waveFormat->nChannels; ++i)
			{
				if(waveFormat->wBitsPerSample == 8)
				{
					amplitude += *((BYTE *) statusInfo.pchNext);
					assert(statusInfo.pchNext != statusInfo.pchEndRead);
					++statusInfo.pchNext;

					bytesRead += 1;
				}
				else if(waveFormat->wBitsPerSample == 16)
				{
					signed short int a = *((unsigned short *) statusInfo.pchNext);
					if(a < 0)
						a = -a;
					unsigned char ash = a >> 7;
					assert(statusInfo.pchNext != statusInfo.pchEndRead);
					++statusInfo.pchNext;
					assert(statusInfo.pchNext != statusInfo.pchEndRead);
					++statusInfo.pchNext;

					//*((BYTE *) &a) = *((BYTE *) statusInfo.pchNext);
					//++statusInfo.pchNext;
					//*(((BYTE *) &a) + 1) = *((BYTE *) statusInfo.pchNext);
					//++statusInfo.pchNext;

					bytesRead += 2;
					amplitude += ash;
				}
				else
					FB_ASSERT(!"Shouldnt get here.");
			}

			//if(waveFormat->nChannels == 2)
			//	amplitude >>= 1;

			if(waveFormat->nSamplesPerSec < 44100 || loopIndex % 2 == 0)
			{
				//if(amplitude > arrayAmplitude)
				//	arrayAmplitude = amplitude;

				sampleBuffer[sampleIndex] = amplitude;
				if(++sampleIndex >= int(sampleBuffer.size()))
					sampleIndex = 0;
			}

			// Do stuff
			if(bytesRead % amplitudeSampleDelta == 0)
			{
				int avg = 0;
				int max = 0;
				for(unsigned int i = 0; i < sampleBuffer.size(); ++i)
				{
					unsigned char v = sampleBuffer[i];
					avg += v;
					if(v > max)
						max = v;
				}

				avg /= sampleBuffer.size();
				array.setSampleAmplitude(index++, (avg / 2) + (max / 2));
				//array.setSampleAmplitude(index++, (2 * avg / 3) + (1 * max / 3));
				//array.setSampleAmplitude(index++, avg);
				//array.setSampleAmplitude(index++, max);
			}
		}

        if(mmioSetInfo(ioHandle, &statusInfo, 0) != MMSYSERR_NOERROR)
		{
			logParseError(fileName, "readAmplitudeArray::mmioSetInfo");
			free();

			return;
		}

		array.update();
	}

	void free()
	{
		if(ioHandle)
		{
			mmioClose(ioHandle, 0);
			ioHandle = 0;
		}
	}
};

WaveReader::WaveReader(const string &file)
{
	scoped_ptr<Data> newData(new Data());
	data.swap(newData);

	data->open(file);
	data->findPosition();
	data->resetRead();
}

WaveReader::~WaveReader()
{
}

void WaveReader::readAmplitudeArray(int tickTime, AmplitudeArray &array)
{
	data->readAmplitudeArray(tickTime, array);
}


#else  // _WIN32


struct WaveReader::Data
{
	string fileName;

	Sound_Sample *ioHandle;
	Uint32 audio_len;

	boost::shared_array<char> buffer;
	boost::scoped_array<char> fileData;

	Data()
	:	ioHandle(0)
	{
	}

	~Data()
	{
		free();
	}

	void open(const string &file)
	{
		fileName = file;
		int size = 0;

		filesystem::FB_FILE *fileCont = filesystem::fb_fopen(file.c_str(), "rb");
		if(fileCont)
		{
			size = filesystem::fb_fsize(fileCont);
			fileData.reset(new char[size]);
			filesystem::fb_fread(fileData.get(), 1, size, fileCont);
			filesystem::fb_fclose(fileCont);

			SDL_RWops *rw = SDL_RWFromMem(fileData.get(), size);
			ioHandle = Sound_NewSample(rw, NULL, NULL, size);
		}

		if(!ioHandle)
		{
			string errorString = "Cannot open wave file: ";
			errorString += file;

			Logger::getInstance()->warning(errorString.c_str());
		}
	}

	void findPosition()
	{
		if(!ioHandle)
			return;

		// FIXME: check correct format etc.
		unimplemented();

		audio_len = Sound_DecodeAll(ioHandle);

		if(ioHandle->flags & SOUND_SAMPLEFLAG_ERROR)
		{
			logParseError(fileName, "findPosition::mmioDescend");
			free();

			return;
		}

		if(ioHandle->flags & SOUND_SAMPLEFLAG_EAGAIN)
		{
			logParseError(fileName, "findPosition::mmioAscend");
			free();

			return;
		}
	}

	void resetRead()
	{
		if(!ioHandle)
			return;

		Sound_Rewind(ioHandle);
		findPosition();
	}

	void readAmplitudeArray(int tickTime, AmplitudeArray &ampArray)
	{
		FB_ASSERT(tickTime >= 10);
		if(!ioHandle)
			return;

		int wBitsPerSample = 0;
		if(ioHandle->actual.format == AUDIO_U8 || ioHandle->actual.format == AUDIO_S8)
			wBitsPerSample = 8;
		else if(ioHandle->actual.format == AUDIO_U16 || ioHandle->actual.format == AUDIO_S16)
			wBitsPerSample = 16;

		if(wBitsPerSample != 8 && wBitsPerSample != 16)
		{
			logParseError(fileName, "readAmplitudeArray::unsupported bits per sample amount");
			free();

			return;
		}

		if(ioHandle->actual.channels != 1 && ioHandle->actual.channels != 2)
		{
			logParseError(fileName, "readAmplitudeArray::unsupported channel amount");
			free();

			return;
		}

		int sampleSize = ioHandle->actual.rate * ioHandle->actual.channels * wBitsPerSample / 8;
		int amplitudeSamplesPerSecond = 1000 / tickTime;
		int amplitudeSampleDelta = sampleSize / amplitudeSamplesPerSecond;
		int amplitudeSampleAmount = ioHandle->buffer_size / amplitudeSampleDelta + 1;

		if(ioHandle->flags & SOUND_SAMPLEFLAG_ERROR)
		{
			logParseError(fileName, "readAmplitudeArray::mmioGetInfo");
			free();

			return;
		}

		ampArray.setSampleAmount(amplitudeSampleAmount);
		int index = 0;

		unsigned int bytesRead = 0;
		int loopIndex = 0;

		vector<unsigned char> sampleBuffer(amplitudeSampleDelta / 2);
		int sampleIndex = 0;

		Uint8 *pchNext = reinterpret_cast<Uint8*>(ioHandle->buffer);
		Uint16 *pchNext16 = reinterpret_cast<Uint16*>(ioHandle->buffer);

		while(bytesRead < ioHandle->buffer_size)
		{
			++loopIndex;

			int amplitude = 0;

			if(ioHandle->actual.format == AUDIO_U8 || ioHandle->actual.format == AUDIO_S8)
			{
				amplitude += *pchNext;
				++pchNext;

				bytesRead += 1;
			}
			else
			{
				signed short int a = *pchNext16;
				if(a < 0)
					a = -a;
				unsigned char ash = a >> 7;

				++pchNext16;

				bytesRead += 2;
				amplitude += ash;
			}

			if(ioHandle->actual.rate < 44100 || loopIndex % 2 == 0)
			{
				sampleBuffer[sampleIndex] = amplitude;
				if(++sampleIndex >= int(sampleBuffer.size()))
					sampleIndex = 0;
			}

			// Do stuff
			if(bytesRead % amplitudeSampleDelta == 0)
			{
				int avg = 0;
				int max = 0;
				for(unsigned int i = 0; i < sampleBuffer.size(); ++i)
				{
					unsigned char v = sampleBuffer[i];
					avg += v;
					if(v > max)
						max = v;
				}

				avg /= sampleBuffer.size();
				ampArray.setSampleAmplitude(index++, (avg / 2) + (max / 2));
				//fprintf(stderr, "%d\n", ((avg / 2) + (max / 2)));
			}
		}

		ampArray.update();
	}

	void free()
	{
		if(ioHandle)
		{
			Sound_FreeSample(ioHandle);
			ioHandle = 0;
		}
	}
};

WaveReader::WaveReader(const string &file)
{
	scoped_ptr<Data> newData(new Data());
	data.swap(newData);

	data->open(file);
	data->findPosition();
	data->resetRead();
}

WaveReader::~WaveReader()
{
}

void WaveReader::readAmplitudeArray(int tickTime, AmplitudeArray &ampArray)
{
	data->readAmplitudeArray(tickTime, ampArray);
}


// fill specified vector with sound data
void WaveReader::decodeAll(std::vector<char> &target)
{
	assert(this != NULL);
	assert(data);
	assert(data->ioHandle != NULL);

	target.resize(data->audio_len);
	memcpy(&target[0], data->ioHandle->buffer, data->audio_len);

}


// return sampling frequency
unsigned int WaveReader::getFreq() const
{
	assert(this != NULL);
	assert(data);
	assert(data->ioHandle != NULL);

	return data->ioHandle->actual.rate;
}


// return number of channels
unsigned int WaveReader::getChannels() const
{
	assert(this != NULL);
	assert(data);
	assert(data->ioHandle != NULL);

    return data->ioHandle->actual.channels;
}


// return number of bits per sample (8 or 16)
unsigned int WaveReader::getBits() const
{
	assert(this != NULL);
	assert(data);
	assert(data->ioHandle != NULL);

	unsigned int ret = 0;
	switch (data->ioHandle->actual.format)
	{
	case AUDIO_U8:
	case AUDIO_S8:
		ret = 8;
		break;

	default:
		ret = 16;
		break;
	}

	return ret;
}


// returns true if wave file succesfully loaded
WaveReader::operator bool() const
{
	assert(this != NULL);

	return (data && (data->ioHandle != NULL));
}


#endif  // _WIN32

} // sfx
