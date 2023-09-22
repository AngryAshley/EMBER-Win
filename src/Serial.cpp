#include "Serial.h"

//char *port_name="\\\\.\\COM4"; //old port name variable

SerialPort::SerialPort(){
    this->connected = false;
}

int SerialPort::connect(std::string portName){
    this->connected = false;

    this->handler = CreateFileA(static_cast<LPCSTR>(portName.c_str()), //static_cast<LPCSTR>(portName)
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    if (this->handler == INVALID_HANDLE_VALUE){
        if (GetLastError() == ERROR_FILE_NOT_FOUND){
            printf("\nSERIAL_ERROR: Handle was not attached, %s not available\n", portName.c_str());
            return 1;
        } else {
            printf("\nSERIAL_ERROR: %s",strerror(GetLastError()));
            return 1;
        }
    } else {
        DCB dcbSerialParameters = {0};
        if (!GetCommState(this->handler, &dcbSerialParameters)) {
            printf("failed to get current serial parameters");
        } else {
            dcbSerialParameters.BaudRate = CBR_1200;
            dcbSerialParameters.ByteSize = 7;
            dcbSerialParameters.StopBits = ONESTOPBIT;
            dcbSerialParameters.Parity = EVENPARITY;
            dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;

            if (!SetCommState(handler, &dcbSerialParameters))
            {
                printf("\nALERT: could not set Serial port parameters\n");
                return 1;
            } else {
                this->connected = true;
                PurgeComm(this->handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
                Sleep(ARDUINO_WAIT_TIME);
                return 0;
            }
        }
    }
    return 0;
}

SerialPort::~SerialPort(){
    if (this->connected){
        this->connected = false;
        CloseHandle(this->handler);
    }
}

int SerialPort::readSerialPort(char *buffer, unsigned int buf_size){
    DWORD bytesRead;
    unsigned int toRead;

    ClearCommError(this->handler, &this->errors, &this->status);

    if (this->status.cbInQue > 0){
        if (this->status.cbInQue > buf_size){
            toRead = buf_size;
        } else toRead = this->status.cbInQue;
    }
    if (ReadFile(this->handler, buffer, toRead, &bytesRead, NULL)) return bytesRead;
    return 0;
}

bool SerialPort::writeSerialPort(char *buffer, unsigned int buf_size){
    DWORD bytesSend;

    if (!WriteFile(this->handler, (void*) buffer, buf_size, &bytesSend, 0)){
        ClearCommError(this->handler, &this->errors, &this->status);
        return false;
    } else return true;
}

bool SerialPort::isConnected(){
    return this->connected;
}


/// Extra Functions ///

void SerialPort::write(std::string input){
    const char *text = input.c_str();
    int len = input.std::string::size();
    for(int i=0;  i<len; i++){
        printf("%c",text[i]);

    };
    char * convert;
    convert = const_cast<char *>(text);
    this->writeSerialPort(convert,len);
}

void SerialPort::print(std::string input){
    printf("<");
    write(input);
    printf(">\n");
    this->writeSerialPort("\r\n",2);
}

std::string SerialPort::readLine(int option, int specialChar){
    char buf;
    buf = 0;
    bool shouldLoop = true;
    bool shouldAdd = true;
    std::string out;
    std::string in ="";
    int amount = 0;

    while(shouldLoop){
        bool shouldPrint = true;
        bool shouldAdd = true;
        //this->readSerialPort(buf,1);
        in = this->getKey();
        if(in.size()<2){
            buf = in.c_str()[0];
        switch(buf){

            case '\r': if(option!=3)this->writeSerialPort("\r\n",2); shouldLoop=false; break;
            case '\n': if(option!=3)this->writeSerialPort("\r\n",2); shouldLoop=false; break;
            case 0: shouldPrint=false; break;
            case '\x7F':
            case '\x08': if(amount>0){
                            out = out.std::string::substr(0, out.std::string::size()-1);
                            this->writeSerialPort("\x08 \x08",3);
                            amount--;
                         }
                         if(option==2)break;
                         buf = 0;
                         shouldPrint=false;
                         shouldAdd=false;
                         break;
        }
        } else {
            shouldAdd=false;
            if(in=="\e[A"||in=="\e[B"||in=="\e[C"||in=="\e[D"){shouldPrint=false;};
            if(shouldPrint)this->write(in);
            //return "";
        }
        if(buf!=0)printf("%02X", buf);
        if(shouldPrint && shouldLoop){
            if(shouldAdd){
                out += buf;
                amount++;
            } else {
                if(amount>0){
                    amount--;
                }
            }
            char retBuf[1];
            retBuf[0]=buf;
            switch(option){
            case 0: this->writeSerialPort(retBuf,1); break;
            case 1: this->writeSerialPort("*",1); break;
            case 2: this->writeSerialPort(" \x08",2); break;
            case 3: break;
            }
            buf=0;
        };
    }

    return out;
}

std::string SerialPort::getKey(){
    char* buf;
    //std::string output ="";
    int i=0;
    buf = (char*) malloc(80);
    memset(buf, NULL, 80);

    std::string output;

    char* out = NULL;
    *buf = NULL;
    while(*buf==NULL){
        i=this->readSerialPort(buf,1);
    }
    std::string _output(1, *buf);
    output += _output;
    //printf(" buf: %02x ", *buf);

    *buf=NULL;
    return output;
}

