//
//  main.cpp
//  tiny_dfu_write
//
//  Created by r-tamura omniment on 2021/01/11.
//

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

bool serial_open(char* port);
bool serial_send(uint8_t* data, uint32_t len = 1);
bool serial_receive(uint8_t* data,uint16_t len = 1);
bool receive_check(uint8_t* rec_data_arr, int len = 1);

void close_handle(bool quit_state);

bool into_dfu(uint8_t* receive_arr);
bool write_que(uint8_t que_number);
bool write_que_addr(uint32_t addr);

bool get_version(uint8_t* receive_data);
bool get_version_protect(uint8_t* receive_data);
bool get_id(uint8_t* receive_data);
bool read_mem_data(uint8_t* receive_data, uint32_t addr, uint8_t len = 255);
bool erase_flash_sector(uint32_t page, uint16_t dellen);
bool write_flash_data(uint8_t* data, uint32_t addr, uint16_t len);
bool write_flash_cycle(FILE* bin, uint64_t bin_size, bool erase_option = false);
bool cmd_go(uint32_t addr, uint8_t* receive_data);

unsigned long BytesWritten;
unsigned long BytesRead;

unsigned int bf = TIOCM_RTS;

//debug
unsigned long count = 0;

int huart;

FILE* bin_file;

int main(int argc, char* argv[])
{
    
    std::cout << "\r\n---RECore DFU Tool with TinyDFU Write v0.1---\r\n";
    std::cout << "---2021 Omniment Inc.---\r\n";

    uint8_t receive_data_arr[256];
    receive_data_arr[0] = 0xff;

    //bool err_flag = false;
    if(argc < 5){
        std::cout << "Argument count error.\r\nコマンドライン引数が不足しています。\r\n";
        return 1;
    }

    printf("Port %s\r\n", argv[1]);
    printf("File %s\r\n", argv[2]);
    printf("Writre Address %s\r\n", argv[3]);
    printf("DevID %s\r\n", argv[4]);
    
    if(argv[1] == NULL){
        std::cout<<"port null error.\r\nポート指定エラー\r\n";
        close_handle(true);
    }

    bin_file = fopen(argv[2],"rb");
    
    if(bin_file == NULL){
        std::cout << "Can't open bin file.\r\n" << "binファイルが開けませんでした。\r\n";
        close_handle(true);
    }

    struct stat statBuf;
    
    stat(argv[2], &statBuf);
    
    uint64_t bin_length = statBuf.st_size;
    rewind(bin_file);

    printf("file size : %llu\r\n", bin_length);
    
    if(serial_open(argv[1])){
        std::cout << "Failed port open. Please check to connect or other apps in use.\r\n"<< "ポートを開けませんでした。 接続、あるいは他のアプリが使用中では無いか確認してください。\r\n";
        close_handle(true);
    }
    
    if (into_dfu(receive_data_arr)) {
        std::cout << "Fail Set DFU Mode\r\n" << "DFUモードに入れませんでした。\r\n";
        close_handle(true);
    }

    if (get_id(receive_data_arr)) {
        std::cout << "Fail get chip data\r\n" << "データの取得に失敗しました。";
        close_handle(true);
    }

    if (receive_data_arr[3] != uint8_t(atoi(argv[4]))) {
        std::cout << "Fail match ChipID\r\n" << "チップIDが一致しませんでした。";
        close_handle(true);
    }

    if (write_flash_cycle(bin_file, bin_length)) {
        close_handle(true);
    }

    if (cmd_go(0x08000000, receive_data_arr)) {
        std::cout << "err run prog\r\n";
        close_handle(true);
    }

    std::cout << "\r\nComplete Write!\r\n";
    close_handle(false);
}

void close_handle(bool quit_state){
    fclose(bin_file);
    close(huart);
    exit(quit_state);
}

bool serial_open(char* port) {
    huart = open(port, O_RDWR|O_NOCTTY);
    
    if(huart < 0){
        //err
        return true;
    }
    
    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    
    tio.c_cflag = CS8|CREAD|CLOCAL|PARENB;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VTIME] = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_ispeed = 115200;
    tio.c_ospeed = 115200;
    
    //受信データクリア
    tcflush(huart, TCIFLUSH);
    
    // デバイスに設定を行う
    tcsetattr(huart, TCSANOW, &tio);
    
    return 0;
}

bool serial_send(uint8_t* data, uint32_t len) {
    size_t send_len = write(huart,data,len);
    
    //printf("\r\nsend bytes %u\r\n", send_len);
    //for (unsigned int i = 0; i < send_len; i++) {
    //    printf("send data : %x\r\n", data[i]);
    //}
    return 0;
}

bool serial_receive(uint8_t* data, uint16_t len) {
    read(huart,&data,len);
    //return(!ReadFile(huart,  data, 1, &BytesRead, NULL));
    //need fix return
    return 0;
}

bool receive_check(uint8_t* rec_data_arr, int len) {
    int receive_length = 0;
    
    clock_t start_time = clock();
    while (receive_length < len) {
        ioctl(huart, FIONREAD, &receive_length);
        clock_t dt = clock() - start_time;
        if (dt > 3000000) {
            std::cout<<"uart timeout\r\n";
            return true;
        }
    }
    //printf("receive len : %d\r\n",receive_length);
    
    long readed_len = read(huart,rec_data_arr,receive_length);
    if (readed_len == 0) {
        return true;
    }
    //printf("data : %x\r\n",rec_data_arr[0]);
    
    if(rec_data_arr[0] != 0x79){
        std::cout<<"return data not good\r\n";
        return true;
    }

    //printf("receive length %d\r\n", receive_length);
    //for (int i = 0; i < receive_length; i++) {
    //    printf("rec %x : %u\r\n", rec_data_arr[i], rec_data_arr[i]);
    //}

    return 0;
}

bool into_dfu(uint8_t* receive_arr) {
    std::cout << "Start DFU Mode\r\n";
    
    //reset rts
    ioctl(huart, TIOCMBIC, &bf);
    
    ioctl(huart, TIOCSDTR);
    sleep(1);
    
    ioctl(huart,TIOCCDTR);
    sleep(3);
    
    //serial受信バッファリセット
    tcflush(huart, TCIFLUSH);
    
    uint8_t send_data = 0x7f;
    if (serial_send(&send_data)) {
        return true;
    }
    
    if (receive_check(receive_arr)) {
        return true;
    }

    return 0;
}

bool write_que(uint8_t que_number) {
    uint8_t que_data[2] = { que_number,  uint8_t(0xff ^ que_number) };
    if (serial_send(que_data,2)) {
        return true;
    }
    return 0;
}

bool write_que_addr(uint32_t addr) {
     uint8_t addr_data[5];
     //uint8_t* addr_data = (uint8_t*)&addr; <-armとエンディアン逆だぁ

     addr_data[3] = addr & 0xff;
     addr_data[2] = (addr >> 8) & 0xff;
     addr_data[1] = (addr >> 8 * 2) & 0xff;
     addr_data[0] = (addr >> 8 * 3) & 0xff;
     addr_data[4] = addr_data[3] ^ addr_data[2] ^ addr_data[1] ^ addr_data[0];

     if (serial_send(addr_data,5)) {
         return true;
     }

     return 0;
}

bool get_version(uint8_t* receive_data) {
    //get chip version
    write_que(0x00);
    return receive_check(receive_data);
}

bool get_version_protect(uint8_t* receive_data) {
    //get version and protect status
    write_que(0x01);
    return receive_check(receive_data);
}

bool get_id(uint8_t* receive_data) {
    //get id
    write_que(0x02);
    return receive_check(receive_data);
}

bool read_mem_data(uint8_t* receive_data, uint32_t addr, uint8_t len) {
    //Read Memory
    write_que(0x11);
    if (receive_check(receive_data)) {
        return true;
    }

    write_que_addr(addr);
    if (receive_check(receive_data)){
        return true;
    }
    
    write_que(len);
    if (receive_check(receive_data, len + 2)) {
        return true;
    }
    return 0;
}

bool erase_flash_sector(uint32_t page, uint16_t dellen) {
    uint8_t receive_data[256];
    uint8_t que_data[7];

    write_que(0x44);
    if (receive_check(receive_data)) {
        return true;
    }

    if (page != 0xffff) {
        if (dellen != 0) {
            que_data[2] = (uint8_t(page >> 8 & 0xff));
        }
        que_data[0] = (dellen >> 8) & 0xff;
        que_data[1] = dellen & 0xff;

        que_data[2] = (page >> 8) & 0xff;
        que_data[3] = page & 0xff;

        que_data[4] = que_data[0] ^ que_data[1] ^ que_data[2] ^ que_data[3];
        if (serial_send(que_data,5)) {
            return true;
        }
    }

    if (receive_check(receive_data)) {
        return true;
    }
    return 0;
}

bool write_flash_data(uint8_t* data, uint32_t addr, uint16_t len) {
    uint8_t receive_data[256];
    //uint8_t send_len = sizeof(data) - 1;
    uint8_t send_len = uint8_t(len-1);
    //printf("send data len : %d\r\n", send_len);

    write_que(0x31);
    if (receive_check(receive_data)) {
        return true;
    }

    write_que_addr(addr);

    if (receive_check(receive_data)) {
        return true;
    }

    uint8_t chk_sum = send_len;
    //uint32_t buf = 0;
    //uint8_t chk_sum;
    
    for (int i = 0; i < len; i++) {
        chk_sum = chk_sum ^ data[i];
    }

    serial_send(&send_len);
    serial_send(data, send_len+1);
    serial_send(&chk_sum);

    if (receive_check(receive_data)) {
        return true;
    }

    return 0;
}

bool write_flash_cycle(FILE* bin, uint64_t bin_size, bool erase_option) {
    std::cout << "Elase Flash ";

    uint64_t erase_page_num = bin_size / 2048;
    if ((bin_size % 2048) != 0) {
        erase_page_num++;
    }

    //erase flash
    for (uint32_t i = 0; i < erase_page_num; i++) {
        if (erase_flash_sector(i, 0)) {
            std::cout << "error erase flash\r\n";
            return true;
        }
        std::cout << "#";
    }
    std::cout << "\r\nErase Complete\r\n";
    
    //write data
    
    std::cout << "Write Data ";
    uint8_t write_data_buffer[256];

    bool write_continue = true;
    uint32_t counter = 0;

    while (write_continue) {
        //printf("send brock %d\r\n", count);
        count++;
        size_t write_length = fread(write_data_buffer, sizeof(uint8_t), 256, bin);
        if (write_length == 256) {
            if (write_flash_data(write_data_buffer, (0x08000000 + (counter * 256)), 256)) {
                return true;
            }
        }
        else if (write_length != 256) {
            if (write_length != 0) {
                if (write_flash_data(write_data_buffer, (0x08000000 + (counter * 256)), write_length)) {
                    return true;
                }
            }
            write_continue = false;
        }
        std::cout << "#";
        counter++;
    }

    return 0;

}


bool cmd_go(uint32_t addr, uint8_t* receive_data) {
    write_que(0x21);
    if (receive_check(receive_data)) {
        return true;
    }

    write_que_addr(addr);
    if (receive_check(receive_data)) {
        return true;
    }
    
    //set rts
    ioctl(huart, TIOCMBIS, &bf);
    //hardreset
    ioctl(huart, TIOCSDTR);
    sleep(1);
    
    ioctl(huart, TIOCSDTR);
    sleep(1);
    
    return 0;
}

/*
def unprotect_rdp():
    rec = bytearray()
    write_que(0x92)
    if receive_check(rec):
        return True
    return receive_check(rec)
 */
