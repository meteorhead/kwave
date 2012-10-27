/***************************************************************************
       PlayBack-OSS.cpp  -  playback device for standard linux OSS
			     -------------------
    begin                : Sat May 19 2001
    copyright            : (C) 2001 by Thomas Eschenbacher
    email                : Thomas.Eschenbacher@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "config.h"
#ifdef HAVE_OSS_SUPPORT

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include <QDir>
#include <QFile>

#include <klocale.h>

#include "libkwave/memcpy.h"
#include "libkwave/ByteOrder.h"
#include "libkwave/CompressionType.h"
#include "libkwave/SampleEncoderLinear.h"

#include "PlayBack-OSS.h"

/** use at least 2^8 = 256 bytes for playback buffer !!! */
#define MIN_PLAYBACK_BUFFER 8

/** use at most 2^16 = 65536 bytes for playback buffer !!! */
#define MAX_PLAYBACK_BUFFER 16

/** highest available number of channels */
#define MAX_CHANNELS 7

// Linux 2.6.24 and above's OSS emulation supports 24 and 32 bit formats
// but did not declare them in <soundcard.h>
#ifndef AFMT_S24_LE
#define AFMT_S24_LE      0x00008000
#endif
#ifndef AFMT_S24_BE
#define AFMT_S24_BE      0x00010000
#endif
#ifndef AFMT_S32_LE
#define AFMT_S32_LE      0x00001000
#endif
#ifndef AFMT_S32_BE
#define AFMT_S32_BE      0x00002000
#endif

#ifndef SNDCTL_DSP_SPEED
#define SNDCTL_DSP_SPEED SOUND_PCM_WRITE_RATE
#endif

#ifndef SNDCTL_DSP_CHANNELS
#define SNDCTL_DSP_CHANNELS SOUND_PCM_WRITE_CHANNELS
#endif

#ifndef SOUND_PCM_SETFMT
#define SOUND_PCM_SETFMT SOUND_PCM_WRITE_BITS
#endif

#ifndef SNDCTL_DSP_SETFMT
#define SNDCTL_DSP_SETFMT SOUND_PCM_SETFMT
#endif

//***************************************************************************
PlayBackOSS::PlayBackOSS()
    :PlayBackDevice(),
    m_device_name(),
    m_handle(-1),
    m_rate(0),
    m_channels(0),
    m_bits(0),
    m_bufbase(0),
    m_buffer(),
    m_raw_buffer(),
    m_buffer_size(0),
    m_buffer_used(0),
    m_encoder(0),
    m_oss_version(-1)
{
}

//***************************************************************************
PlayBackOSS::~PlayBackOSS()
{
    close();
}

//***************************************************************************
QString PlayBackOSS::open(const QString &device, double rate,
                          unsigned int channels, unsigned int bits,
                          unsigned int bufbase)
{
    qDebug("PlayBackOSS::open(device=%s,rate=%0.1f,channels=%u,"\
	"bits=%u, bufbase=%u)", device.toLocal8Bit().data(), rate, channels,
	bits, bufbase);

    m_device_name = device;
    m_rate        = rate;
    m_channels    = channels;
    m_bits        = bits;
    m_bufbase     = bufbase;
    m_buffer_size = 0;
    m_buffer_used = 0;
    m_handle      = 0;

    // prepeare for playback by opening the sound device
    // and initializing with the proper settings
    m_handle = ::open(m_device_name.toLocal8Bit(), O_WRONLY | O_NONBLOCK);
    if (m_handle == -1) {
	QString reason;
	switch (errno) {
	    case ENOENT:
	    case ENODEV:
	    case ENXIO:
	    case EIO:
		reason = i18n("I/O error. Maybe the driver\n"\
		"is not present in your kernel or it is not\n"\
		"properly configured.");
		break;
	    case EBUSY:
		reason = i18n(
		"The device is busy. Maybe some other application is\n"\
		"currently using it. Please try again later.\n"\
		"(Hint: you might find out the name and process ID of\n"\
		"the program by calling: \"fuser -v %1\"\n"\
		"on the command line.)",
		m_device_name.section('|',0,0));
		break;
	    default:
		reason = strerror(errno);
	}
	return reason;
    }

    // the device was opened in non-blocking mode to detect if it is
    // busy or not - but from now on we need blocking mode again
    ::fcntl(m_handle, F_SETFL, fcntl(m_handle, F_GETFL) & ~O_NONBLOCK);
    if (fcntl(m_handle, F_GETFL) & O_NONBLOCK) {
	// resetting O:NONBLOCK failed
	return i18n("The device '%1' cannot be opened "\
	            "in the correct mode.",
	            m_device_name.section('|',0,0));
    }

    // query OSS driver version
    m_oss_version = 0x030000;
#ifdef OSS_GETVERSION
    ioctl(m_handle, OSS_GETVERSION, &m_oss_version);
#endif

    // query OSS driver version for debugging purposes
    m_oss_version = -1;
#ifdef OSS_GETVERSION
    ioctl(m_handle, OSS_GETVERSION, &m_oss_version);
#endif
    int format;
    switch (m_bits) {
	case  8: format = AFMT_U8;     break;
	case 24: format = AFMT_S24_LE; break;
	case 32: format = AFMT_S32_LE; break;
	default: format = AFMT_S16_LE;
    }

    // number of bits per sample
    int oldformat = format;
    if ((ioctl(m_handle, SNDCTL_DSP_SETFMT, &format) == -1) ||
        (format != oldformat)) {
	return i18n("%1 bits per sample are not supported", m_bits);
    }

    // channels selection
    if ((ioctl(m_handle, SNDCTL_DSP_CHANNELS, &m_channels) == -1) ||
        (format != oldformat)) {
	return i18n("%1 channels playback is not supported", m_channels);
    }

    // playback rate, we allow up to 10% variation
    int int_rate = static_cast<int>(m_rate);
    if ((ioctl(m_handle, SNDCTL_DSP_SPEED, &int_rate) == -1) ||
	(int_rate < 0.9 * m_rate) || (int_rate > 1.1 * m_rate)) {
	return i18n("Playback rate %1 Hz is not supported", int_rate);
    }
    m_rate = int_rate;

    // buffer size
    Q_ASSERT(bufbase >= MIN_PLAYBACK_BUFFER);
    Q_ASSERT(bufbase <= MAX_PLAYBACK_BUFFER);
    if (bufbase < MIN_PLAYBACK_BUFFER) bufbase = MIN_PLAYBACK_BUFFER;
    if (bufbase > MAX_PLAYBACK_BUFFER) bufbase = MAX_PLAYBACK_BUFFER;
    if (ioctl(m_handle, SNDCTL_DSP_SETFRAGMENT, &bufbase) == -1) {
	return i18n("Unusable buffer size: %1", 1 << bufbase);
    }

    // get the real buffer size in bytes
    ioctl(m_handle, SNDCTL_DSP_GETBLKSIZE, &m_buffer_size);

    // create the sample encoder
    // we assume that OSS is always little endian
    if (m_encoder) delete m_encoder;

    switch (m_bits) {
	case 8:
	    m_encoder = new Kwave::SampleEncoderLinear(
		SampleFormat::Unsigned, 8, LittleEndian);
	    break;
	case 24:
	    if (m_oss_version >= 0x040000) {
		m_encoder = new Kwave::SampleEncoderLinear(
		SampleFormat::Signed, 24, LittleEndian);
		break;
	    } // else:
	    /* FALLTHROUGH */
	case 32:
	    if (m_oss_version >= 0x040000) {
		m_encoder = new Kwave::SampleEncoderLinear(
		    SampleFormat::Signed, 32, LittleEndian);
		break;
	    }
	    // else:
	    /* FALLTHROUGH */
	default:
	    m_encoder = new Kwave::SampleEncoderLinear(
		SampleFormat::Signed, 16, LittleEndian);
	    break;
    }

    Q_ASSERT(m_encoder);
    if (!m_encoder) return i18n("Out of memory");

    // resize the raw buffer
    m_raw_buffer.resize(m_buffer_size);

    // resize our buffer (size in samples) and reset it
    m_buffer_size /= m_encoder->rawBytesPerSample();
    m_buffer.resize(m_buffer_size);

    return 0;
}

//***************************************************************************
int PlayBackOSS::write(const Kwave::SampleArray &samples)
{
    Q_ASSERT (m_buffer_used <= m_buffer_size);
    if (m_buffer_used > m_buffer_size) {
	qWarning("PlayBackOSS::write(): buffer overflow ?!");
	m_buffer_used = m_buffer_size;
	flush();
	return -EIO;
    }

    // number of samples left in the buffer
    unsigned int remaining = samples.size();
    unsigned int offset    = 0;
    while (remaining) {
	unsigned int length = remaining;
	if (m_buffer_used + length > m_buffer_size)
	    length = m_buffer_size - m_buffer_used;

	MEMCPY(&(m_buffer[m_buffer_used]),
	       &(samples[offset]),
	       length * sizeof(sample_t));
	m_buffer_used += length;
	offset        += length;
	remaining     -= length;

	// write buffer to device if it has become full
	if (m_buffer_used >= m_buffer_size)
	    flush();
    }

    return 0;
}

//***************************************************************************
void PlayBackOSS::flush()
{
    if (!m_buffer_used || !m_encoder) return; // nothing to do

    // convert into byte stream
    unsigned int bytes = m_buffer_used * m_encoder->rawBytesPerSample();
    m_encoder->encode(m_buffer, m_buffer_used, m_raw_buffer);

    int res = 0;
    if (m_handle)
	res = ::write(m_handle, m_raw_buffer.data(), bytes);
    if (res) perror(__FUNCTION__);

    m_buffer_used = 0;
}

//***************************************************************************
int PlayBackOSS::close()
{
    flush();

    // close the device handle
    if (m_handle) ::close(m_handle);

    // get rid of the old encoder
    if (m_encoder) delete m_encoder;
    m_encoder = 0;

    return 0;
}

//***************************************************************************
static bool addIfExists(QStringList &list, const QString &name)
{
    QFile file;

    if (name.contains("%1")) {
	// test for the name without suffix first
	addIfExists(list, name.arg(""));

	// loop over the list and try until a suffix does not exist
	for (unsigned int index=0; index < 64; index++)
	    addIfExists(list, name.arg(index));
    } else {
	// check a single name
	file.setFileName(name);
	if (!file.exists())
	    return false;

	if (!list.contains(name))
	    list.append(name);
    }

    return true;
}

//***************************************************************************
static void scanFiles(QStringList &list, const QString &dirname,
                      const QString &mask)
{
    QStringList files;
    QDir dir;

    dir.setPath(dirname);
    dir.setNameFilters(mask.split(' '));
    dir.setFilter(QDir::Files | QDir::Writable | QDir::System);
    dir.setSorting(QDir::Name);
    files = dir.entryList();

    for (QStringList::Iterator it=files.begin(); it != files.end(); ++it) {
	QString devicename = dirname + QDir::separator() + (*it);
	addIfExists(list, devicename);
    }
}

//***************************************************************************
static void scanDirectory(QStringList &list, const QString &dir)
{
    scanFiles(list, dir, "dsp*");
    scanFiles(list, dir, "*audio*");
    scanFiles(list, dir, "adsp*");
    scanFiles(list, dir, "dio*");
    scanFiles(list, dir, "pcm*");
}

//***************************************************************************
QStringList PlayBackOSS::supportedDevices()
{
    QStringList list, dirlist;

    scanDirectory(list, "/dev");
    scanDirectory(list, "/dev/snd");
    scanDirectory(list, "/dev/sound");
    scanFiles(dirlist, "/dev/oss", "[^.]*");
    foreach (QString dir, dirlist)
	scanDirectory(list, dir);
    list.append("#EDIT#");
    list.append("#SELECT#");

    return list;
}

//***************************************************************************
QString PlayBackOSS::fileFilter()
{
    QString filter;

    if (filter.length()) filter += "\n";
    filter += QString("dsp*|") + i18n("OSS playback device (dsp*)");

    if (filter.length()) filter += "\n";
    filter += QString("adsp*|") + i18n("ALSA playback device (adsp*)");

    if (filter.length()) filter += "\n";
    filter += QString("*|") + i18n("Any device (*)");

    return filter;
}

//***************************************************************************
void PlayBackOSS::format2mode(int format, int &compression,
                              int &bits, SampleFormat &sample_format)
{
    switch (format) {
	case AFMT_MU_LAW:
	    compression   = AF_COMPRESSION_G711_ULAW;
	    sample_format = SampleFormat::Signed;
	    bits          = 16;
	    break;
	case AFMT_A_LAW:
	    compression   = AF_COMPRESSION_G711_ALAW;
	    sample_format = SampleFormat::Unsigned;
	    bits          = 16;
	    break;
	case AFMT_IMA_ADPCM:
	    compression   = AF_COMPRESSION_MS_ADPCM;
	    sample_format = SampleFormat::Signed;
	    bits          = 16;
	    break;
	case AFMT_U8:
	    compression   = AF_COMPRESSION_NONE;
	    sample_format = SampleFormat::Unsigned;
	    bits          = 8;
	    break;
	case AFMT_S16_LE:
	case AFMT_S16_BE:
	    compression   = AF_COMPRESSION_NONE;
	    sample_format = SampleFormat::Signed;
	    bits          = 16;
	    break;
	case AFMT_S8:
	    compression   = AF_COMPRESSION_NONE;
	    sample_format = SampleFormat::Signed;
	    bits          = 8;
	    break;
	case AFMT_U16_LE:
	case AFMT_U16_BE:
	    compression   = AF_COMPRESSION_NONE;
	    sample_format = SampleFormat::Unsigned;
	    bits          = 16;
	    break;
	case AFMT_MPEG:
	    compression   = static_cast<int>(CompressionType::MPEG_LAYER_II);
	    sample_format = SampleFormat::Signed;
	    bits          = 16;
	    break;
#if 0
	case AFMT_AC3: /* Dolby Digital AC3 */
	    compression   = SampleFormat::Unknown;
	    sample_format = 0;
	    bits          = 16;
	    break;
#endif
	case AFMT_S24_LE:
	case AFMT_S24_BE:
	    if (m_oss_version >= 0x040000) {
		compression   = AF_COMPRESSION_NONE;
		sample_format = SampleFormat::Signed;
		bits          = 24;
		break;
	    }
	case AFMT_S32_LE:
	case AFMT_S32_BE:
	    if (m_oss_version >= 0x040000) {
		compression   = AF_COMPRESSION_NONE;
		sample_format = SampleFormat::Signed;
		bits          = 32;
		break;
	    }
	default:
	    compression   = -1;
	    sample_format = SampleFormat::Unknown;
	    bits          = -1;
    }

}

//***************************************************************************
int PlayBackOSS::openDevice(const QString &device)
{
    int fd = m_handle;

//     qDebug("PlayBackOSS::openDevice(%s)", device.local8Bit().data());
    if (!device.length()) return -1;

    if (fd <= 0) {
	// open the device in case it's not already open
	fd = ::open(device.toLocal8Bit(), O_WRONLY | O_NONBLOCK);
	if (fd <= 0) {
	    qWarning("PlayBackOSS::openDevice('%s') - "\
		 "failed, errno=%d (%s)",
		 device.toLocal8Bit().data(),
		 errno, strerror(errno));
	} else {
	    // we use blocking mode
	    ::fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
	    // query OSS driver version
	    m_oss_version = -1;
#ifdef OSS_GETVERSION
	    ioctl(fd, OSS_GETVERSION, &m_oss_version);
#endif
	}
    }
    if (fd <= 0) {
	qWarning("PlayBackOSS::openDevice('%s') - "\
	         "failed, errno=%d (%s)",
	         device.toLocal8Bit().data(),
	         errno, strerror(errno));
    }

    return fd;
}

//***************************************************************************
QList<unsigned int> PlayBackOSS::supportedBits(const QString &device)
{
    QList<unsigned int> bits;
    bits.clear();
    int err = -1;
    int mask = AFMT_QUERY;
    int fd;

//     qDebug("PlayBackOSS::supportedBits(%s)", device.local8Bit().data());

    fd = openDevice(device);
    if (fd >= 0) {
	err = ::ioctl(fd, SNDCTL_DSP_GETFMTS, &mask);
	if (err < 0) {
	    qWarning("PlayBackOSS::supportedBits() - "\
	             "SNDCTL_DSP_GETFMTS failed, "\
	             "fd=%d, result=%d, error=%d (%s)",
	             fd, err, errno, strerror(errno));
	}
    }

    // close the device if *we* opened it
    if ((fd != m_handle) && (fd >= 0)) ::close(fd);

    if (err < 0) return bits;

    // mask out all modes that do not match the current compression
    const int compression = AF_COMPRESSION_NONE;
    for (unsigned int bit=0; bit < (sizeof(mask) << 3); bit++) {
	if (!mask & (1 << bit)) continue;

	// format is supported, split into compression, bits, sample format
	int c, b;
	SampleFormat s;
	format2mode(1 << bit, c, b, s);
	if (b < 0) continue; // unknown -> skip

	// take the mode if compression matches and it is not already known
	if ((c == compression) && !(bits.contains(b))) {
	    bits += b;
	}
    }

    return bits;
}

//***************************************************************************
int PlayBackOSS::detectChannels(const QString &device,
                                unsigned int &min, unsigned int &max)
{
    int fd, t, err;

    min = 0;
    max = 0;

    fd = openDevice(device);
    if (fd < 0) return -1;

    // find the smalles number of tracks, limit to MAX_CHANNELS
    for (t=1; t < MAX_CHANNELS; t++) {
	int real_tracks = t;
	err = ioctl(fd, SNDCTL_DSP_CHANNELS, &real_tracks);
	Q_ASSERT(real_tracks == t);
	if (err >= 0) {
	    min = real_tracks;
	    break;
	}
    }
    if (t >= MAX_CHANNELS) {
	// no minimum track number found :-o
	qWarning("no minimum track number found, err=%d",err);
	// close the device if *we* opened it
	if ((fd != m_handle) && (fd >= 0)) ::close(fd);
	return err;
    }

    // find the highest number of tracks, start from MAX_CHANNELS downwards
    for (t = MAX_CHANNELS; t >= static_cast<int>(min); t--) {
	int real_tracks = t;
	err = ioctl(fd, SNDCTL_DSP_CHANNELS, &real_tracks);
	Q_ASSERT(real_tracks == t);
	if (err >= 0) {
	    max = real_tracks;
	    break;
	}
    }
    max = t;
//     qDebug("PlayBackOSS::detectTracks, min=%u, max=%u", min, max);

    // close the device if *we* opened it
    if ((fd != m_handle) && (fd >= 0)) ::close(fd);
    return 0;
}

#endif /* HAVE_OSS_SUPPORT */

//***************************************************************************
//***************************************************************************
