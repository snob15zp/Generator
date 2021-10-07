#ifndef __FS_H_
#define __FS_H_

/**
* File transfer protocol
* 
* File transfered by items, which contains:
* 
* 1. File name length
* 2. File name, up to 16 symbols, with \0 ending
* 3. Write offset, 31 bit of offset value indicate that this item is last item of transfering
* 4. File content.
* 
* In-short: L (length) F (filename) O (offset) V (value)
* 
* Used files:
* 1. mcu.fw  -> Current mcu firmware for rom-ba (file bootloader)
* 2. FPGA.rbf -> FPGA firmware
* 3. *.txt -> Frequency files
* 4. freq.pls -> Contain frequency file names, to arrange freq file list
* 
* After succesful writing 'freq.pls', which should written last, calls 'playlist_cb_t' callback
* That callback should be set by 'spiffs_on_write_playlist_done()'
*/


/**
* Playlist callback format
*/
typedef void (*fwrite_done_cb_t)();

/**
* Initialize spiffs
* mount and format if needed file-system
* @return Error or 0 if not there is no error
*/
int spiffs_init(void);

/**
* Erase all files from FS
* @return Error or 0 if not there is no error
*/
int spiffs_erase_all(void);

/**
* Erase file by extension
*/
int spiffs_erase_by_ext(const char* ext); 

/**
* Set playlist done callback
*/
void spiffs_on_write_playlist_done(fwrite_done_cb_t cb);

/**
* Set bq28z610 firmware write done callback  
*/
void spiffs_on_write_bq28z610_done(fwrite_done_cb_t cb);

/**
* Set tps65987 firmware write done callback
*/
void spiffs_on_write_tps65987_done(fwrite_done_cb_t cb);

/**
* Set callback that called on removing playlist file (erase_by_file_ext(*.pls))
*/
void spiffs_on_playlist_remove(fwrite_done_cb_t cb);

/**
* Set callback that called on formating flash
*/
void spiffs_on_flash_format(fwrite_done_cb_t cb);


/**
* Formating filesystem
*/
int spiffs_format_flash();

/**
* Remove 'fw.bf' if it found
*/
void spiffs_rm_fw();
#endif //__FS_H_