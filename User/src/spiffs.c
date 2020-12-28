#include "spiffs.h"
#include "w25qxx.h"

spiffs fs;

static u8_t spiffs_work_buf[SPIFFS_CFG_LOG_PAGE_SZ() * 2];
static u8_t spiffs_fds[32 * 4];
static u8_t spiffs_cache_buf[(SPIFFS_CFG_LOG_PAGE_SZ() + 32) * 4];

static s32_t w25_flash_read(int addr, int size, char *buf)
{
    W25qxx_ReadBytes(buf, addr, size);
    return SPIFFS_OK;
}

static s32_t w25_flash_write(int addr, int size, char *buf)
{
    W25qxx_WritePage(buf, addr >> 8, addr & 0xff , size);
    return SPIFFS_OK;
}

static s32_t w25_flash_erase(int addr, int size)
{
    if(size == 0x1000)
    {
        W25qxx_EraseSector(addr / 0x1000);
    }
    
    return SPIFFS_OK;    
}

/**
* @brief Write part of file to fs
* @param filename Filename, null-terminated string
* @param fname_len Filename length
* @param offset of writing data
* @param data Data itself
* @param data_len Length of data
* @return 0 - if there no error
*/
int spiffs_write_file_part(const char *filename, size_t fname_len, uint32_t offset, uint8_t *data, size_t data_len)
{
    spiffs_file fd = SPIFFS_open(&fs, filename, SPIFFS_O_CREAT | SPIFFS_RDWR, 0);

    if(SPIFFS_errno(&fs) < 0)
    {
        return SPIFFS_errno(&fs);
    }
    
    /*
    if (SPIFFS_lseek(&fs, fd, offset, SPIFFS_SEEK_SET) < 0)
    {
        return SPIFFS_errno(&fs);
    }
    */
   
    if (SPIFFS_write(&fs, fd, data, data_len) < 0)
    {
        return SPIFFS_errno(&fs);
    }

    SPIFFS_close(&fs, fd);

    return 0;
}

/**
* @brief Erase freq files from FS
* @return Error, if happend, otherwise 0
*/
int spiffs_erase_all()
{
    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;
    int res;

    SPIFFS_opendir(&fs, "/", &d);
    while ((pe = SPIFFS_readdir(&d, pe)))
    {
        if (SPIFFS_remove(&fs, (char *)pe->name) < 0)
        {
            return SPIFFS_errno(&fs);
        }
    }

    SPIFFS_closedir(&d);
    return 0;
}

/**
* Initialize and configure spiffs 
*/
int spiffs_init()
{
    spiffs_config cfg;

    //W25qxx_EraseChip();
    
    cfg.hal_read_f = w25_flash_read;
    cfg.hal_write_f = w25_flash_write;
    cfg.hal_erase_f = w25_flash_erase;

    
    int32_t res = SPIFFS_mount(&fs,
                               &cfg,
                               spiffs_work_buf,
                               spiffs_fds,
                               sizeof(spiffs_fds),
                               spiffs_cache_buf,
                               sizeof(spiffs_cache_buf),
                               0);
   
                               
    if (res == SPIFFS_ERR_NOT_A_FS)
    {
        SPIFFS_unmount(&fs);
        
        SPIFFS_format(&fs);
        return SPIFFS_mount(&fs,
                            &cfg,
                            spiffs_work_buf,
                            spiffs_fds,
                            sizeof(spiffs_fds),
                            spiffs_cache_buf,
                            sizeof(spiffs_cache_buf),
                            0);
    }

    return res;
}


/**
* Callback from write file method in freemodbus
* @param buf Input data
* @param len Length of data
* @return Error code
*/
int on_modbus_write_file(uint8_t* buf, size_t len)
{
    uint8_t fname_len = buf[0];
    char fname[17] = {};
    if(buf[0] >= sizeof(fname))
    {
        return 0x81;
    }
    
    char* ptr = (char*) &buf[1];
    memcpy(fname, ptr, fname_len);        
    ptr += fname_len;
    
    uint32_t offset = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
    ptr += sizeof(uint32_t);
        
    int res = spiffs_write_file_part(fname, ptr - (char*)buf, offset, ptr, len - (ptr - (char*)buf)); 
    
    return res;
}

