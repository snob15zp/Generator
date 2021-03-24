
#include "fs.h"
#include "stm32g0xx.h"
#include "spiffs.h"
#include "tim3.h"
#include "Spi1.h"
#include "SL_CommModbus.h"
#include "fs.h"
#include "aes.h"

//#define FLASH_TEST
//#define FILE_SHA_TEST

#ifdef FILE_SHA_TEST
#include "sha256.h"
#endif

#define W25_CHIP_ERASE 0xC7
#define W25_READ_CMD 0x0b
#define W25_WRITE_CMD 2
#define W25_STATUS_CMD 5
#define W25_WRITE_ENA_CMD 6

#define W25_4K_ERASE_CMD 0x20
#define W25_4K_ERASE_TIME 50

#define W25_32K_ERASE_CMD 0x52
#define W25_32K_ERASE_TIME 300

#define W25_64K_ERASE_CMD 0xd8
#define W25_64K_ERASE_TIME 500

#define W25_ERASE_TIMEOUT 3000

spiffs fs;
static fwrite_done_cb_t playlist_write_done_cb = NULL;
static fwrite_done_cb_t bq28z610_write_done_cb = NULL;
static fwrite_done_cb_t tps65987_write_done_cb = NULL;
static fwrite_done_cb_t playlist_remove_cb = NULL;
static fwrite_done_cb_t format_flash_cb = NULL;

/**
* File encryption KEY
*/
static const uint8_t encrypt_key[] = { 0x3a, 0xf5, 0x4c, 0x68, 0xaa, 0x0a, 0x65, 0xf2, 0xb2, 0x2f, 0xd5, 0x33, 0x05, 0xb9, 0xad, 0x96 }; 

/**
* File encryption IV
* Should be reseted at every new item of data (when offset = 0)
*/
static uint8_t encrypt_iv[16] = {};

extern uint8_t spiDispCapture;
__attribute__((aligned(4))) static u8_t spiffs_work_buf[SPIFFS_CFG_LOG_PAGE_SZ() * 2];
__attribute__((aligned(4))) static u8_t spiffs_fds[32 * 4];
__attribute__((aligned(4))) static u8_t spiffs_cache_buf[(SPIFFS_CFG_LOG_PAGE_SZ() + 32) * 4];

/**
* Retrieve flash memory status
*/
static uint8_t get_status()
{
    spi_cs_on();
    spi_transfer(W25_STATUS_CMD);
    volatile uint8_t status = spi_transfer(0);
    spi_cs_off();
    
    return status;
}

static s32_t wait_done(uint16_t time)
{
    uint32_t timeout = 0;
    while(1)
    {
        uint8_t status = get_status();
        
        if( status == 0xff)
        {
            delay_ms(time);
            continue;
        }
        
        if((status & 1) == 0)
        {
            break;
        }
        
        delay_ms(time);
        timeout += time;
        if(timeout > W25_ERASE_TIMEOUT)
        {
            return SPIFFS_ERR_ERASE_FAIL;
        }
    }
    
    return SPIFFS_OK;
}


static void set_write_ena()
{
    uint32_t timeout = 0;
    while(1)
    {
        uint8_t status = get_status();
        
        if( status == 0xff)
        {
            delay_ms(10);
            continue;
        }
        
        if(status & 2)
        {
            return;
        }
              
        timeout += 1;
        if(timeout > W25_ERASE_TIMEOUT)
        {
            return;
        }
    
        spi_cs_on();
        spi_transfer(W25_WRITE_ENA_CMD);
        spi_cs_off();
    }
}


/**
* Erase entire chip
*/
static void flash_chip_erase()
{
    set_write_ena();
    spi_cs_on();
    spi_transfer(W25_CHIP_ERASE);
    spi_cs_off();
    delay_ms(25000);
    wait_done(50);
    
}

static s32_t w25_flash_read(uint32_t addr, uint32_t size, uint8_t *buf)
{
  
    spi_cs_on();
    spi_transfer(W25_READ_CMD);
    spi_transfer(addr >> 16);
    spi_transfer(addr >> 8);
    spi_transfer(addr & 0xff);
    spi_transfer(0);
    
    for(size_t i = 0; i< size; i++)
    {
        buf[i] = spi_transfer(0);
    }
    
    spi_cs_off();
    
    return SPIFFS_OK;
}

/**
* Write flash page
*/
static void w25_write_page(uint32_t addr, size_t size, uint8_t* buf)
{
    set_write_ena();

    spi_cs_on();
    spi_transfer(W25_WRITE_CMD);
    spi_transfer(addr >> 16);
    spi_transfer(addr >> 8);
    spi_transfer(addr & 0xff);
    
    for(size_t i = 0; i< size; i++)
    {
        spi_transfer(buf[i]);
    }
    
    spi_cs_off();
    delay_ms(1);
    wait_done(5);
}

/**
* Write data into flash, up-to 256 bytes
*/
static s32_t w25_flash_write(uint32_t addr, uint32_t size, uint8_t *buf)
{
   
    do
    {
        uint8_t p_addr = addr & 0xff;
        size_t p_size = ((p_addr + size) >= 0x100) ? (0x100 - p_addr) : size;
        w25_write_page(addr, p_size, buf);
        addr += p_size;
        buf += p_size;
        size -= p_size;
    }
    while(size);
    
    return SPIFFS_OK;
}

/**
* Erase flash page/sector/block 
* @param addr Erase address
* @apram size Size which should be erased
*/
static s32_t w25_flash_erase(uint32_t addr, uint32_t size)
{
    
    set_write_ena();
    
    uint16_t time = W25_4K_ERASE_TIME;
    spi_cs_on();
    if(size <= 4096)
    {
         spi_transfer(W25_4K_ERASE_CMD);
    } 
    else if(size <= 32768)
    {
        spi_transfer(W25_32K_ERASE_CMD);
        time = W25_32K_ERASE_TIME;
    }
    else if(size <= 65536)
    {
        spi_transfer(W25_64K_ERASE_CMD);
        time = W25_64K_ERASE_TIME;
    }
    
    spi_transfer(addr >> 16);
    spi_transfer(addr >> 8);
    spi_transfer(addr & 0xff);
    
    spi_cs_off();
    delay_ms(time);  
    return wait_done(time);
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
static int spiffs_write_file_part(const char *filename, size_t fname_len, uint32_t offset, uint8_t *data, size_t data_len, uint8_t last)
{
    int32_t res = 0;
    static spiffs_file fd;
    if(offset == 0)
    {
        if (fd > 0)
        {
            SPIFFS_close(&fs, fd);
        }

        fd = SPIFFS_open(&fs, filename, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
    }
    else
    {
        spiffs_stat s;
        res = SPIFFS_fstat(&fs, fd, &s);
        
        if(res == 0 && offset > s.size)
        {
            res = SPIFFS_ERR_END_OF_OBJECT;
        }
        
        if((res == 0) && (offset != s.size))
        {
            if (SPIFFS_lseek(&fs, fd, offset, SPIFFS_SEEK_SET) < 0)
            {
                res = SPIFFS_errno(&fs);
            }
        }
    }
    
    if( res == 0)
    {
        if (SPIFFS_write(&fs, fd, data, data_len) < 0)
        {
            res = SPIFFS_errno(&fs);
        }
    }
    
    SPIFFS_fflush(&fs,fd);
    
    if (last)
    {
       SPIFFS_close(&fs, fd);
    }
    
    return res;
}


void spiffs_on_write_playlist_done(fwrite_done_cb_t cb)
{
    playlist_write_done_cb = cb;
}


void spiffs_on_write_bq28z610_done(fwrite_done_cb_t cb)
{
    bq28z610_write_done_cb = cb;
}


void spiffs_on_write_tps65987_done(fwrite_done_cb_t cb)
{
    tps65987_write_done_cb = cb;
}


void spiffs_on_playlist_remove(fwrite_done_cb_t cb)
{
    playlist_remove_cb = cb;
}


void spiffs_on_flash_format(fwrite_done_cb_t cb)
{
    format_flash_cb = cb;
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
    int res = 0;

    SPIFFS_opendir(&fs, "/", &d);
    while ((pe = SPIFFS_readdir(&d, pe)))
    {
        if (SPIFFS_remove(&fs, (char *)pe->name) < 0)
        {
            res = SPIFFS_errno(&fs);
            break;
        }
    }

    SPIFFS_closedir(&d);
    
    
    if (playlist_remove_cb != NULL)
    {
        playlist_remove_cb();
    }
    
    return res;
}


/**
* @brief Erase file by extension
* @return Error, if happend, otherwise 0
*/
int spiffs_erase_by_ext(const char* ext)
{
    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;
    int res = 0;

    SPIFFS_opendir(&fs, "/", &d);
    while ((pe = SPIFFS_readdir(&d, pe)))
    {
        char* fext = strchr((const char*)pe->name,'.');
        if(fext == NULL)
        {
            continue;
        }
            
        if (strncasecmp(fext+1, ext, strlen(fext)-1) == 0) 
        {
            if (SPIFFS_remove(&fs, (char *)pe->name) < 0)
            {
                break;
                res = SPIFFS_errno(&fs);
            }
        }
    }
    
    SPIFFS_closedir(&d);
    
    if (res == 0 && playlist_remove_cb != NULL && strncasecmp("pls",ext,3) == 0 )
    {
        playlist_remove_cb();
    }
    
    return res;
}

int spiffs_format_flash()
{
    
    if (format_flash_cb != NULL)
    {
        format_flash_cb();
    }
    SPIFFS_unmount(&fs); 
    flash_chip_erase();
    NVIC_SystemReset();
    return 0;
}


#ifdef FILE_SHA_TEST
void sha_file_test()
{
    uint8_t buf[512] = {};
    SHA256_CTX sha_ctx = {};
    uint8_t sha_hash[32] = {};
    const uint8_t expected_sha[32] = {0xb3,0xa5,0xa9,0x38,0x14,0xed,0xd7,0x3a,0xc6,0x38,0x6c,0x78,0x0c,0xff,0xdd,0xec,0xa1,0x2f,0x7a,0x1f,0xec,0x85,0x9c,0xe1,0xe5,0xe2, 0x01,0x55,0x6a, 0xa4,0xbf, 0x3d};
        
    spiffs_file fd;      
    fd = SPIFFS_open(&fs, "sha_tst.txt", SPIFFS_RDONLY, 0);
    size_t readed = SPIFFS_read(&fs, fd, (void *)&buf, sizeof(buf));
    
	if (readed != 430)
	{
        while(1);
	}
    
    sha256_init(&sha_ctx);
    sha256_update(&sha_ctx,buf,readed);
    sha256_final(&sha_ctx,(uint8_t*)sha_hash);
    
    if(memcmp(expected_sha, sha_hash,sizeof(expected_sha)) != 0)
    {
        while(1);
    }
    
    SPIFFS_close(&fs, fd);
}
#endif 


#ifdef FLASH_TEST
const char test_text[] = "Thank you Jack,\
With the oscilloscope,i can see that the frame is shifted.\
The thing is i don't have the information about the frame at t0, because the master sending request continuously. So, the frame visualized using oscilloscope shows (0x47, 0xAF, 0x41, 0x05, 0x05)instead of (0x00, 0x41,0x05, 0x47, 0xAF),the frame send by the Slave is 0x410547AF.\
The explanation that i find is at t0, the Slave is synchronized to the Master only with the two last bytes 0x41, 0x05 shifted by Two, the two first bytes, i imagine that are 0 . Then the 0x47, 0xAF still in the Tx FIFO(this explains the FIFO level is usually at 2 bytes). At t1, the Slave sends 0x47, 0xAF, then 0x41, 0x05 placed between time in the FIFO. This why in MISO line, i can see (0x47, 0xAF, 0x41, 0x05, 0x05)but i can't understand why 0x05 is redundant.\
My problem could the latency when writing to Tx FIFO and to the DR register ?\
Note: I measured the period of the Chip Select and is about 12µs\
May be it's not sufficient to the Slave to respond quickly ?\
If hope that u can have an idea or explanation to this issue,\
Best Regards";
#endif

/**
* Initialize and configure spiffs 
*/
int spiffs_init()
{
    spiffs_config cfg;
   
    GPIOB->BSRR = GPIO_BSRR_BS2;
    
    cfg.hal_read_f = w25_flash_read;
    cfg.hal_write_f = w25_flash_write;
    cfg.hal_erase_f = w25_flash_erase;

    spi_cs_off(); 
    
   
    delay_ms(50);
    
    //flash_chip_erase();
    //set_write_ena();
    
    
#ifdef FLASH_TEST
    uint32_t init_addr = 0xc0;
    char tmp[sizeof(test_text)] = {};
    w25_flash_write(init_addr,sizeof(test_text),test_text);
    w25_flash_read(init_addr,sizeof(test_text),tmp);
    if(memcmp(tmp,test_text,sizeof(test_text)))
    {
        while(1);
    }
    w25_flash_erase(init_addr,sizeof(test_text));
    w25_flash_read(init_addr,sizeof(test_text),tmp);

    for(size_t i = 0; i< sizeof(test_text); i++)
    {
      if(tmp[i] != 0xff)
      {
          while(1);
      }
    }
#endif
    
    
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
        if (format_flash_cb != NULL)
        {
            format_flash_cb();
        }

        SPIFFS_unmount(&fs);  
        SPIFFS_format(&fs);
        res = SPIFFS_mount(&fs,
                            &cfg,
                            spiffs_work_buf,
                            spiffs_fds,
                            sizeof(spiffs_fds),
                            spiffs_cache_buf,
                            sizeof(spiffs_cache_buf),
                            0);                    
    }

#ifdef FILE_SHA_TEST
    sha_file_test();
#endif
    
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
    uint8_t last_item = 0;
    uint8_t encrypted = 0;
    
    char fname[18] = {};    
    if(buf[0] >= sizeof(fname) - 1)
    {
        return 0x81;
    }
    
    uint8_t* ptr = &buf[1];
    memcpy(&fname[0], ptr, fname_len);        
    ptr += fname_len;
    
    uint32_t offset = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
    last_item = (offset & (1 << 31)) == (1 << 31);
    encrypted = (offset & (1 << 30)) == (1 << 30);
    
    offset &= ~((1<<31)|(1<<30));
    
    ptr += sizeof(uint32_t);
    len -= (ptr - buf);
    
    if(encrypted)
    {
        if(offset == 0)
        {
            memset(encrypt_iv,0,sizeof(encrypt_iv));
        }
        
        if ((len % sizeof(encrypt_key)) != 0)
        {
            len -= len % sizeof(encrypt_key);
        }

        for(uint8_t i = 0; i < len; i+= sizeof(encrypt_key))
        {
            aes128_dec((uint8_t*)encrypt_key, ptr + i, AES_CBC, encrypt_iv);
        }
        
        while(len > 0 && ptr[--len] == 0)
        {
        }
        
        len++;
    }
    
    int res = spiffs_write_file_part(fname, fname_len, offset, ptr, len, last_item);
    
    if (last_item)
    {
        MODBUScommLastTime = SystemTicks - USBcommPause + USBcommPauseErase;
    }
			
    if (res == 0 && last_item)
    {
        if(playlist_write_done_cb != NULL &&
            strncmp(fname,"freq.pls",fname_len) == 0)
        {
            playlist_write_done_cb();
        }
        
        if(bq28z610_write_done_cb != NULL &&
            strncmp(fname,"bq28z610.srec",fname_len) == 0)
        {
            bq28z610_write_done_cb();
        }
        
        if(tps65987_write_done_cb != NULL &&
            strncmp(fname,"tps65987.bin",fname_len) == 0)
        {
            tps65987_write_done_cb();
        }
    }
    
    return res;
}
