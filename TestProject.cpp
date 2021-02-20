#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 256


void  decToBinary(int n , char &c) 
{
   // array to store binary number 
    int binaryNum[8]; 
  
    // counter for binary array 
    int i = 0; 
    while (n > 0) { 
          // storing remainder in binary array 
        binaryNum[i] = n % 2; 
        n = n / 2; 
        i++; 
    } 
  
    // printing binary array in reverse order 
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j]==1)
            c = c | 1u << j;
    }
 } 

// #define SYS_CALL
// ============================================================================
class fsInode {
    int fileSize;
    int block_in_use;

    int *directBlocks;
    int singleInDirect;
    int num_of_direct_blocks;
    int block_size;


    public:
    fsInode(int _block_size, int _num_of_direct_blocks) {
        fileSize = 0; 
        block_in_use = 0; 
        block_size = _block_size;
        num_of_direct_blocks = _num_of_direct_blocks;
        directBlocks = new int[num_of_direct_blocks];
		assert(directBlocks);
        for (int i = 0 ; i < num_of_direct_blocks; i++) {   
            directBlocks[i] = -1;
        }
        singleInDirect = -1;

    }

    int getFileSize(){
        return fileSize;
    }

    void setFileSize(int data){
        fileSize=data;
    }

    int getBlock_in_use(){
        return block_in_use;
    }

    void setBlock_in_use(int data){
        block_in_use=data;
    }

    int getSingleInDirect(){
        return singleInDirect;
    }

    void setSingleInDirect(int data){
        singleInDirect=data;
    }

    int* getDirectBlocks(){
        return directBlocks;
    }

    void setDirectBlocks(int index,int data){
        directBlocks[index]=data;
    }

    ~fsInode() { 
        delete directBlocks;
    }

};

// ============================================================================
class FileDescriptor {
    pair<string, fsInode*> file;
    bool inUse;

    public:
    FileDescriptor(string FileName, fsInode* fsi) {
        file.first = FileName;
        file.second = fsi;
        inUse = true;

    }

    string getFileName() {
        return file.first;
    }
    void setFileNameToEmptyString(){
        file.first="";
    }

    fsInode* getInode() {
        
        return file.second;
    }

    bool isInUse() { 
        return (inUse); 
    }
    void setInUse(bool _inUse) {
        inUse = _inUse ;
    }
};
 
#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================
class fsDisk {
    FILE *sim_disk_fd;
 
    bool is_formated=false;

	// BitVector - "bit" (int) vector, indicate which block in the disk is free
	//              or not.  (i.e. if BitVector[0] == 1 , means that the 
	//             first block is occupied. 
    int BitVectorSize;
    int *BitVector;

    // Unix directories are lists of association structures, 
    // each of which contains one filename and one inode number.
    map<string, fsInode*>  MainDir ; 

    // OpenFileDescriptors --  when you open a file, 
	// the operating system creates an entry to represent that file
    // This entry number is the file descriptor. 
    vector< FileDescriptor > OpenFileDescriptors;

    int direct_enteris=0;
    int block_size=0;

    public:
    // ------------------------------------------------------------------------
    fsDisk() {
        sim_disk_fd = fopen( DISK_SIM_FILE , "r+" );
        assert(sim_disk_fd);
        for (int i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd );
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);

    }

    ~fsDisk(){
        int i=0;
        delete(BitVector);
        fclose(sim_disk_fd);
        for(i=0;i<OpenFileDescriptors.size();i++)
            if(OpenFileDescriptors[i].getFileName().compare("")!=0)
                delete(OpenFileDescriptors[i].getInode());
            
        
        }
	
    // ------------------------------------------------------------------------
    void listAll() {
        int i = 0;    
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it->getFileName() <<  " , isInUse: " << it->isInUse() << endl; 
            i++;
        }
        char bufy;
        cout << "Disk content: '" ;
        for (i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fread(  &bufy , 1 , 1, sim_disk_fd );
             cout << bufy;              
        }
        cout << "'" << endl;

    }
 
    // ------------------------------------------------------------------------
    void fsFormat( int blockSize =4, int direct_Enteris_ = 3 ) {

        if(is_formated==true){
            cout<<"Disk already formated"<<endl;
            return;
        }
            
        this->block_size=blockSize;
        this->direct_enteris=direct_Enteris_;
        this->BitVectorSize=DISK_SIZE/block_size;
        this->BitVector= new int[BitVectorSize];
        this->is_formated=true;
        cout<<"FORMAT DISK: number of blocks: "<< BitVectorSize <<endl;
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName) {
 
        if(is_formated==false)
            return -1;

        int i=0;
        for(i=0;i<OpenFileDescriptors.size();i++)
            if(fileName.compare(OpenFileDescriptors[i].getFileName())==0){
                return -1;
            }

        fsInode* file=new fsInode(block_size,direct_enteris);
        FileDescriptor file2(fileName,file);
        OpenFileDescriptors.push_back(file2);
        MainDir.insert(pair<string,fsInode*>(fileName,file));
   
        for(i=0;i<OpenFileDescriptors.size();i++)
            if(fileName.compare(OpenFileDescriptors[i].getFileName())==0){
                return i;
            }
        return i;

    }

    // ------------------------------------------------------------------------
    int OpenFile(string fileName) {

        int i=0;
        for(i=0;i<OpenFileDescriptors.size();i++)
            if(fileName.compare(OpenFileDescriptors[i].getFileName())==0){
                if(OpenFileDescriptors[i].isInUse()==false){
                    OpenFileDescriptors[i].setInUse(true);
                    return i;
                }
            }

            return -1;
    }  

    // ------------------------------------------------------------------------
    string CloseFile(int fd) {

        if(OpenFileDescriptors.size()>fd){
            if(OpenFileDescriptors[fd].isInUse()==true){
                OpenFileDescriptors[fd].setInUse(false);
                
                return OpenFileDescriptors[fd].getFileName();
            }
        }
        return "-1";      
    }
    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char *buf, int len ) {
         
        //in case disk is not formated
        if(is_formated==false){
            cout<<"Disk is not formated"<<endl;
            return -1;
        }    

        // file descriptor does not exist
        if(OpenFileDescriptors.size()<fd+1){
            cout<<"File descriptor does not exist"<<endl;
            return -1;
        }
     
        // file has been deleted
        if(OpenFileDescriptors[fd].getFileName().compare("")==0){
            cout<<"File has been deleted"<<endl;
            return -1;
        }

        // the file is closed
        if(OpenFileDescriptors[fd].isInUse()==false){
            cout<<"File is closed"<<endl;
            return -1;
        }

        // length of data is too big for a file
        if(len>(direct_enteris+block_size)*block_size){
            cout<<"File max data exceeded"<<endl;
            return -1;
        }

        int i=0,freeBits=0;
        for(i=0;i<BitVectorSize;i++)
            if(BitVector[i]==0)
                freeBits++; 

        int direct_blocks=0;
        int indirect_blocks=0;
        int indirect_len=0;
       
        for(i=len;i<DISK_SIZE-len;i++)
            buf[i]='\0';

        // write to a file first time
        if(OpenFileDescriptors[fd].getInode()->getBlock_in_use()==0){

            // file is only in direct blocks
            if(len<=(direct_enteris*block_size)){

                direct_blocks=len/block_size;
                if(len%block_size!=0)
                    direct_blocks++;

                if(freeBits<direct_blocks){
                    cout<<"Insufficient disk memory"<<endl;
                    return -1;
                }

                OpenFileDescriptors[fd].getInode()->setBlock_in_use(direct_blocks);        
            }
            // file is also in single indirect block
            else{
                direct_blocks=direct_enteris;
                indirect_len=(len-direct_enteris*block_size);
                indirect_blocks=indirect_len/block_size;
                if(indirect_len%block_size!=0)
                    indirect_blocks++;
                
                if(freeBits<direct_blocks+indirect_blocks){
                    cout<<"Insufficient disk memory"<<endl;
                    return -1;
                }

                OpenFileDescriptors[fd].getInode()->setBlock_in_use(direct_blocks+indirect_blocks);    
            }

            OpenFileDescriptors[fd].getInode()->setFileSize(len);

            // set free blocks to the file direct blocks
            i=0;
            int j=0;
            while(i<direct_blocks){
                for(j=0;j<BitVectorSize;j++)
                     if(BitVector[j]==0){
                         BitVector[j]=1;
                        OpenFileDescriptors[fd].getInode()->setDirectBlocks(i,j);
                        break;
                    }
                i++;
            }
            
            // write to disk all the direct blocks data
            for(i=0;i<direct_blocks;i++){

                fseek(sim_disk_fd,OpenFileDescriptors[fd].getInode()->getDirectBlocks()[i]*block_size,SEEK_SET);
                fwrite(buf,sizeof(char),block_size,sim_disk_fd);
                buf=buf+block_size;
            }

            // in case where we have also indirect blocks
            if(indirect_blocks!=0){

                for(j=0;j<BitVectorSize;j++)
                    if(BitVector[j]==0){
                        BitVector[j]=1;
                        OpenFileDescriptors[fd].getInode()->setSingleInDirect(j);
                        break;
                    }

                // set blocks for indirect data + write to disk the indirect data
                i=0;
                char temp;
                int binaryToDec=0;
                while(i<indirect_blocks){

                    for(j=0;j<BitVectorSize;j++)
                    if(BitVector[j]==0){
                        BitVector[j]=1;
                        temp='\0';
                        decToBinary(j,temp);
                        fseek(sim_disk_fd,OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size+i,SEEK_SET);
                        fwrite(&temp,sizeof(char),1,sim_disk_fd);
                        binaryToDec=(int)temp;
                        fseek(sim_disk_fd,binaryToDec*block_size,SEEK_SET);
                        fwrite(buf,sizeof(char),block_size,sim_disk_fd);
                        buf=buf+block_size;
                        break;
                    }
                    i++;
                }
                
            }
                
        }

        // in case we add data to a file already exist
        else{

            int total_len=len+OpenFileDescriptors[fd].getInode()->getFileSize();

            if(total_len>(direct_enteris+block_size)*block_size){
                cout<<"File max data exceeded"<<endl;
                return -1;
            }

            int all_blocks=0;

            // in case we are still only in direct blocks
            if(total_len<=direct_enteris*block_size){

                all_blocks=total_len/block_size;
                if(total_len%block_size!=0)
                    all_blocks++;

                direct_blocks=all_blocks-OpenFileDescriptors[fd].getInode()->getBlock_in_use();    

                if(freeBits<all_blocks-OpenFileDescriptors[fd].getInode()->getBlock_in_use()){
                    cout<<"Insufficient disk memory"<<endl;
                    return -1;
                }    

                OpenFileDescriptors[fd].getInode()->setBlock_in_use(all_blocks);
                
                i=all_blocks-direct_blocks;
                int j=0;
                while(i<all_blocks){

                    for(j=0;j<BitVectorSize;j++)
                        if(BitVector[j]==0){
                            BitVector[j]=1;
                            OpenFileDescriptors[fd].getInode()->setDirectBlocks(i,j);
                            break;
                        }
                    i++;
                }

                int start_block=OpenFileDescriptors[fd].getInode()->getFileSize()/block_size;
                int offset=OpenFileDescriptors[fd].getInode()->getFileSize()%block_size;

                for(i=start_block;i<all_blocks;i++){

                    fseek(sim_disk_fd,OpenFileDescriptors[fd].getInode()->getDirectBlocks()[i]*block_size+offset,SEEK_SET);
                    fwrite(buf,sizeof(char),block_size-offset,sim_disk_fd);
                    buf=buf+(block_size-offset);
                    offset=0;
                }              
            }

            // in case we add data and move from direct to indirect blocks
            else if(OpenFileDescriptors[fd].getInode()->getFileSize()<=(direct_enteris*block_size)){


                direct_blocks=direct_enteris-OpenFileDescriptors[fd].getInode()->getBlock_in_use();
                indirect_len=(total_len-direct_enteris*block_size);
                indirect_blocks=indirect_len/block_size;
                if(indirect_len%block_size!=0)
                    indirect_blocks++;

                all_blocks=total_len/block_size;
                if(total_len%block_size!=0)
                    all_blocks++; 

                if(freeBits<all_blocks-OpenFileDescriptors[fd].getInode()->getBlock_in_use()){
                    cout<<"Insufficient disk memory"<<endl;
                    return -1;
                }    

                OpenFileDescriptors[fd].getInode()->setBlock_in_use(all_blocks);


                // add data to direct blocks first
                i=direct_enteris-direct_blocks;
                int j=0;
                while(i<direct_enteris){

                    for(j=0;j<BitVectorSize;j++)
                        if(BitVector[j]==0){
                            BitVector[j]=1;
                            OpenFileDescriptors[fd].getInode()->setDirectBlocks(i,j);
                            break;
                        }
                    i++;
                }

                int start_block=OpenFileDescriptors[fd].getInode()->getFileSize()/block_size;
                int offset=OpenFileDescriptors[fd].getInode()->getFileSize()%block_size;

                for(i=start_block;i<direct_enteris;i++){

                    fseek(sim_disk_fd,OpenFileDescriptors[fd].getInode()->getDirectBlocks()[i]*block_size+offset,SEEK_SET);
                    fwrite(buf,sizeof(char),block_size-offset,sim_disk_fd);
                    buf=buf+(block_size-offset);
                    offset=0;
                }

                //// add data to indirect blocks////

                // set indirect block
                for(j=0;j<BitVectorSize;j++)
                    if(BitVector[j]==0){
                        BitVector[j]=1;
                        OpenFileDescriptors[fd].getInode()->setSingleInDirect(j);
                        break;
                    }

                // set blocks for indirect data + write to disk the indirect data
                i=0;
                char temp;
                int binaryToDec=0;
                while(i<indirect_blocks){

                    for(j=0;j<BitVectorSize;j++)
                    if(BitVector[j]==0){
                        BitVector[j]=1;
                        temp='\0';
                        decToBinary(j,temp);
                        fseek(sim_disk_fd,OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size+i,SEEK_SET);
                        fwrite(&temp,sizeof(char),1,sim_disk_fd);
                        binaryToDec=(int)temp;
                        fseek(sim_disk_fd,binaryToDec*block_size,SEEK_SET);
                        fwrite(buf,sizeof(char),block_size,sim_disk_fd);
                        buf=buf+block_size;
                        break;
                    }
                    i++;
                }
            }

            // we already have indirect blocks and we add more data
            else{

                all_blocks=total_len/block_size;
                if(total_len%block_size!=0)
                    all_blocks++;

                indirect_blocks=all_blocks-OpenFileDescriptors[fd].getInode()->getBlock_in_use();                     

                if(freeBits<all_blocks-OpenFileDescriptors[fd].getInode()->getBlock_in_use()){
                    cout<<"Insufficient disk memory"<<endl;
                    return -1;
                }    

                OpenFileDescriptors[fd].getInode()->setBlock_in_use(all_blocks);

                char temp='\0';
                int binaryToDec=0;

                // a block still have internal fragmentation
                if(OpenFileDescriptors[fd].getInode()->getFileSize()%block_size!=0){

                int start_block=(OpenFileDescriptors[fd].getInode()->getFileSize()-(block_size*direct_enteris))/block_size;
                int offset=(OpenFileDescriptors[fd].getInode()->getFileSize()-(block_size*direct_enteris))%block_size;


                fseek(sim_disk_fd,OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size+start_block,SEEK_SET);
                fread(&temp,sizeof(char),1,sim_disk_fd);
                binaryToDec=(int)temp;

                fseek(sim_disk_fd,binaryToDec*block_size+offset,SEEK_SET);
                fwrite(buf,sizeof(char),block_size-offset,sim_disk_fd);
                buf+=block_size-offset;
                }

                int j=0;
                i=(OpenFileDescriptors[fd].getInode()->getFileSize()-(direct_enteris*block_size))/block_size;

                while(i<=indirect_blocks){

                    for(j=0;j<BitVectorSize;j++)
                    if(BitVector[j]==0){
                        BitVector[j]=1;
                        temp='\0';
                        decToBinary(j,temp);
                        fseek(sim_disk_fd,OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size+i,SEEK_SET);
                        fwrite(&temp,sizeof(char),1,sim_disk_fd);
                        binaryToDec=(int)temp;
                        fseek(sim_disk_fd,binaryToDec*block_size,SEEK_SET);
                        fwrite(buf,sizeof(char),block_size,sim_disk_fd);
                        buf=buf+block_size;
                        break;
                    }
                    i++;
                }
            }

        OpenFileDescriptors[fd].getInode()->setFileSize(total_len);
        }       
    }
    // ------------------------------------------------------------------------
    int DelFile( string fileName ) {

           if(is_formated==false)
            return -1;

        int i=0,j=0;
        for(i=0;i<OpenFileDescriptors.size();i++)
            // check if such file exists by its name
            if(fileName.compare(OpenFileDescriptors[i].getFileName())==0){

                // free BitVector bits for direct blocks
                for(j=0;j<direct_enteris;j++){
                    if(OpenFileDescriptors[i].getInode()->getDirectBlocks()[j]!=-1)
                        BitVector[OpenFileDescriptors[i].getInode()->getDirectBlocks()[j]]=0;
                }
                // free BitVector bits for indirect blocks
                if(OpenFileDescriptors[i].getInode()->getSingleInDirect()!=-1){
                    char temp;
                    int binaryToDec=0;

                    for(j=0;j<OpenFileDescriptors[i].getInode()->getBlock_in_use()-direct_enteris;j++){
                        fseek(sim_disk_fd,OpenFileDescriptors[i].getInode()->getSingleInDirect()*block_size+j,SEEK_SET);
                        fread(&temp,sizeof(char),1,sim_disk_fd);
                        BitVector[binaryToDec=(int)temp]=0;
                    }
                    BitVector[OpenFileDescriptors[i].getInode()->getSingleInDirect()]=0;
                }

                delete(OpenFileDescriptors[i].getInode());
                OpenFileDescriptors[i].setInUse(false);
                OpenFileDescriptors[i].setFileNameToEmptyString();
                MainDir.erase(fileName);

                return i;
            }
                
        return -1;

    }
    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len ) { 
      
     //in case disk is not formated
    if(is_formated==false){
        cout<<"Disk is not formated"<<endl;
        return -1;
    }    

    // file descriptor is not exist
    if(OpenFileDescriptors.size()<fd){
        cout<<"File descriptor does not exist"<<endl;
        return -1;
    }

    // file has been deleted
        if(OpenFileDescriptors[fd].getFileName().compare("")==0){
            cout<<"File has been deleted"<<endl;
            return -1;
        }


    // the file is closed
    if(OpenFileDescriptors[fd].isInUse()==false){
        cout<<"File is closed"<<endl;
        return -1;
    }

    if(len>(direct_enteris*block_size)*block_size)
        len=(direct_enteris*block_size)*block_size;

    int i=0;

    for(i=0;i<DISK_SIZE;i++)
        buf[i]='\0';
    bool offset_flag;

    int blocks_to_read=len/block_size;
    if(len%block_size!=0)
        blocks_to_read++;

    int only_direct_blocks=0;

    // if only Direct blocks
    if(blocks_to_read<=direct_enteris){

        only_direct_blocks=blocks_to_read;
        offset_flag=true;
    }

    // also indirect blocks    
    else{
        only_direct_blocks=direct_enteris;
        offset_flag=false;
    }
        
    for(i=0;i<only_direct_blocks;i++){

        int offset=0;
        if(i+1==only_direct_blocks && len%block_size!=0 && offset_flag==true)
        offset=block_size-(len%block_size);

        fseek(sim_disk_fd,OpenFileDescriptors[fd].getInode()->getDirectBlocks()[i]*block_size,SEEK_SET);
        fread(buf,sizeof(char),block_size-offset,sim_disk_fd);
        buf+=block_size;
    }

    blocks_to_read-=direct_enteris;

    // indirect blocks
    if(blocks_to_read>0){
        i=0;
        char temp;
        int binaryToDec=0;
        while(i<blocks_to_read){
        
            fseek(sim_disk_fd,OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size+i,SEEK_SET);
            fread(&temp,sizeof(char),1,sim_disk_fd);
            binaryToDec=(int)temp;

            int offset=0;
            if(i+1==blocks_to_read && len%block_size!=0)
                offset=block_size-(len%block_size);

            fseek(sim_disk_fd,binaryToDec*block_size,SEEK_SET);
            fread(buf,sizeof(char),block_size-offset,sim_disk_fd);
            buf+=block_size;
            i++;
        }
    }
}

};
    
int main() {
    int blockSize; 
	int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read; 
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while(1) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
				delete fs;
				exit(0);
                break;

            case 1:  // list-file
                fs->listAll(); 
                break;
          
            case 2:    // format
                cin >> blockSize;
				cin >> direct_entries;
                fs->fsFormat(blockSize, direct_entries);
                break;
          
            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            
            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
             
            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd); 
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
           
            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );             
                break;
          
            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile: " << str_to_read << endl;               
                break;
           
            case 8:   // delete file 
                 cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }

} 