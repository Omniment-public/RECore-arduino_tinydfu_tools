import sys
import time
import serial
import os

receive_data = bytearray()

print("\r\n---RECore DFU Writer 0.1---")
print("---2020 Omniment Inc.---\r\n")

print("Port:"+sys.argv[1])
print("File Path:"+sys.argv[2])
print("Address:"+sys.argv[3])

ser = serial.Serial(sys.argv[1],115200,parity=serial.PARITY_EVEN)

file_path = sys.argv[2]

hex_file = open(file_path, 'rb')
hex_size = os.path.getsize(file_path)

write_addr = sys.argv[3]

# AutoDFU DFUモード移行
def into_dfu():
    rec = bytearray()
    print("Start DFU Mode")
    ser.dtr=False
    ser.rts=False
    ser.dtr=True
    time.sleep(0.2)
    ser.rts=False
    ser.dtr=False
    ser.trscts = False
    ser.dsrdtr = False
    time.sleep(0.2)
    ser.write(bytes([0x7f]))
    if receive_check(rec):
        return True
    return False


def receive_check(recd,len = 1):
    tt = time.time()
    while ser.in_waiting < len:
        dt = time.time() - tt
        if dt > 3:
            break
    
    if ser.in_waiting != 0:
        receive_data.clear()
        for i in ser.read(ser.in_waiting):
            recd.append(i)
        #print(recd.hex())
        if recd[0] != 0x79:
            print("err receive data")
            return True
        return False
    else:
        print("timeout")
        return True


def write_que(que):
    que_data = bytearray([0x00]*2)
    que_data[0] = que
    que_data[1] = (0xff^que_data[0])
    #print(ser.write(que_data))
    ser.write(que_data)


def write_que_addr(addr):
    addr_data = bytearray([0x00]*5)
    addr_data[3] = addr & 0xff
    addr_data[2] = (addr >> 8) & 0xff
    addr_data[1] = (addr >> 8*2) & 0xff
    addr_data[0] = (addr >> 8*3) & 0xff
    addr_data[4] = addr_data[3] ^ addr_data[2] ^ addr_data[1] ^ addr_data[0]
    #print(ser.write(addr_data))
    ser.write(addr_data)


def get_version(rec):
    #get version
    write_que(0x00)
    return receive_check(rec)


def get_version_protect(rec):
    #get version
    write_que(0x01)
    return receive_check(rec)


def get_id(rec):
    #get id
    write_que(0x02)
    return receive_check(rec,5)


def read_mem_data(rec,addr,len = 255):
    #Read Memory
    write_que(0x11)
    if receive_check(rec):
        return True
    
    write_que_addr(addr)
    if receive_check(rec):
        return True
        
    write_que(len)
    if receive_check(rec,len+2):
        return True

def erase_flash_sector(page,dellen):
    rec = bytearray()
    write_que(0x44)
    if receive_check(rec):
        return True
    
    if page != 0xffff:
        if dellen == 0:
            que_data = bytearray([0x00]*5)
        else:
            que_data = bytearray([0x00]*7)
            que_data[2] = (page >> 8) & 0xff
        #dellen = dellen + 1;
        #dellen = dellen;
        que_data[0] = (dellen >> 8) & 0xff
        que_data[1] = dellen & 0xff
        
        que_data[2] = (page >> 8) & 0xff
        que_data[3] = page & 0xff
        
        que_data[4] = que_data[0] ^ que_data[1] ^ que_data[2] ^ que_data[3]
        #print(que_data)
        #print(ser.write(que_data))
        ser.write(que_data)
    
    if receive_check(rec):
        return True


def write_flash_data(data,addr):
    rec = bytearray()
    send_len = bytes([len(data)-1])
    #print(send_len)
    #print("send code")
    write_que(0x31)
    if receive_check(rec):
        return True
    
    #print("send addr")
    write_que_addr(addr)
    if receive_check(rec):
        return True
    
    chk_sum = send_len
    buf = bytes([0x00])
    for senddata in data:
        buf = bytes([senddata])
        chk_sum = bytes([(chk_sum[0] ^ buf[0])])
    
    #print(chk_sum)
    #print(type(chk_sum))
    
    #print("send length")
    #print( ser.write(bytes([send_len[0]])) )
    ser.write(bytes([send_len[0]]))
    
    #print("send data")
    #print( ser.write(data) )
    ser.write(data)
    
    #print("send chksum")
    #print( ser.write(bytes([chk_sum[0]])))
    ser.write(bytes([chk_sum[0]]))
    
    if receive_check(rec):
        return True
    return False


def write_flash_cycle(hex):
    hex.seek(0)
    pb = "Elase Flash "
    erase_page_num = hex_size // 2048
    if (hex_size % 2048) != 0:
        erase_page_num+=1
    
    write_data_num = hex_size // 256
    write_data_fraction = hex_size % 256
    
    #erase
    for i in range(erase_page_num):
        if erase_flash_sector(i,0):
            print("error erase flash")
            return True
        pb = pb + "#"
        print(pb,end="\r")
    
    #write 256 brock
    print("")
    pb = "Write Data "
    for i in range(write_data_num):
        write_data_buffer = hex.read(256)
        if write_flash_data(write_data_buffer, (0x08000000 + (i*256))):
            print("error write flash")
            return True
        pb = pb + "#"
        print(pb,end="\r")
    
    #write fraction
    if write_data_fraction != 0:
        fraction_pos = 0x08000000 + ((write_data_num)*256) 
        if write_flash_data(hex.read(write_data_fraction),fraction_pos):
            print("error write flash")
            return True
        pb = pb + "#"
        print(pb,end="\r")
    print()
    
    return False


def unprotect_rdp():
    rec = bytearray()
    write_que(0x92)
    if receive_check(rec):
        return True
    return receive_check(rec)

def cmd_go(addr):
    rec = bytearray()
    write_que(0x21)
    if receive_check(rec):
        return True
    write_que_addr(addr)
    if receive_check(rec):
        return True


#こっから

if into_dfu() != False:
    print("\r\nFail DFU Mode\r\nPlease check port select or connection")
    #sys.exit("\r\nFail DFU Mode\r\nPlease check port select or connection")
    #ser.close()

if get_id(receive_data) != False:
    print("\r\nFail Get ID\r\nPlease check port select or connection")
    #sys.exit("\r\nFail Get ID\r\nPlease check port select or connection")
    #ser.close()

if receive_data[3] != 72:
    print("\r\nFail Chip Model\r\nPlease check Board")
    #sys.exit("\r\nFail Chip Model\r\nPlease check Board")
    #ser.close()

#read_mem_data(receive_data,0x08000000)
if write_flash_cycle(hex_file) != False:
    print("\r\nFail Write Flash\r\nPlease check Board")
    #sys.exit("\r\nFail Write Flash\r\nPlease check Board")
    #ser.close()

print("Write Complete")
print("Start Prog")
cmd_go(0x08000000)

ser.close()
