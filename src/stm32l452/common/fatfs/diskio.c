/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

#include "boxlib/flash.h"

/* Definitions of physical drive number for each drive */
#define DEV_EXTFLASH 0

#define PAGESIZE FLASHPAGESIZE


uint8_t g_diskState[FF_VOLUMES] = {STA_NOINIT};

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	switch (pdrv) {
	case DEV_EXTFLASH :
		return g_diskState[DEV_EXTFLASH];
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

//FlashEnable must be called before
DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	switch (pdrv) {
	case DEV_EXTFLASH :
		if (g_diskState[DEV_EXTFLASH] & STA_NOINIT) {
			if ((FlashReady()) && (FlashPagesizePowertwoGet()) &&
			    (FlashBlocksizeGet() <= DISK_BLOCKSIZE)) {
				g_diskState[DEV_EXTFLASH] = 0;
			}
		}
		return g_diskState[DEV_EXTFLASH];
	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	switch (pdrv) {
	case DEV_EXTFLASH:
		if (FlashRead(DISK_RESERVEDOFFSET + sector * DISK_BLOCKSIZE, buff, count * DISK_BLOCKSIZE)) {
			return RES_OK;
		} else {
			return RES_ERROR;
		}
	}
	return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	switch (pdrv) {
	case DEV_EXTFLASH:
		if (FlashWrite(DISK_RESERVEDOFFSET + sector * DISK_BLOCKSIZE, buff, count * DISK_BLOCKSIZE)) {
			return RES_OK;
		} else {
			return RES_ERROR;
		}
	}
	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	switch (pdrv) {
	case DEV_EXTFLASH:
		if ((cmd == CTRL_SYNC) || (cmd == CTRL_TRIM)) {
			return RES_OK;
		}
		if (cmd == GET_SECTOR_COUNT) {
			uint32_t sectors = (FlashSizeGet() - DISK_RESERVEDOFFSET) / DISK_BLOCKSIZE;
			*((LBA_t*)buff) = sectors;
			return RES_OK;
		}
		if (cmd == GET_SECTOR_SIZE) {
			*((WORD*)buff) = DISK_BLOCKSIZE;
			return RES_OK;
		}
		if (cmd == GET_BLOCK_SIZE) {
			*(DWORD*)buff = DISK_BLOCKSIZE;
			return RES_OK;
		}
	}
	return RES_PARERR;
}

#if (FF_FS_NORTC == 0)

DWORD get_fattime(void)
{
	return 1; //TODO: Access to RTC
}

#endif