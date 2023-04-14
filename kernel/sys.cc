#include "sys.h"
#include "stdint.h"
#include "idt.h"
#include "debug.h"
#include "threads.h"
#include "process.h"
#include "machine.h"
#include "ext2.h"
#include "elf.h"
#include "libk.h"
#include "file.h"
#include "heap.h"
#include "shared.h"
#include "kernel.h"

int SYS::exec(const char* path,
              int argc,
              const char* argv[]
) {
    using namespace gheith;
    auto file = root_fs->find(root_fs->root,path); 
    if (file == nullptr) {
        return -1;
    }
    if (!file->is_file()) {
        return -1;
    }

    ElfHeader hdr;

    file->read(0,hdr);

    uint32_t magic = hdr.maigc0;
    if(magic != 0x7f) return -1;
    magic = hdr.magic1;
    if(magic != 'E') return -1;
    magic = hdr.magic2;
    if(magic != 'L') return -1;
    magic = hdr.magic3;
    if(magic != 'F') return -1;
    magic = hdr.entry;
    if(0x80000000 > magic || magic > 0xFFFFFFFF) return -1;
    magic += (hdr.phentsize * hdr.phnum);
    magic += (hdr.shentsize * hdr.shnum);
    if(0x80000000 > magic || magic > 0xFFFFFFFF) return -1;

    uint32_t sp = 0xefffe000;

    // Copy arguments
    // Clear the address space
    //MISSING();
    // NOTE: 32bit aligned i.e. 4 bytes i.e. 4 chars
    uint32_t wordaddrs[argc];

    for(int i = argc - 1; i >= 0; i--) {
        uint32_t spaceneeded = K::strlen(argv[i]);
        spaceneeded++; // add 1 to curr length for pad bit
        if(spaceneeded % 4 != 0) spaceneeded += (4 - (spaceneeded % 4));
        sp -= spaceneeded;
        memcpy((uint32_t *) sp, argv[i], spaceneeded);
        wordaddrs[i] = sp;
    }
    for(int i = argc - 1; i >= 0; i--) {
        sp -= 4;
        *((uint32_t *) sp) = wordaddrs[i];
    }
    sp -= 4;
    *((uint32_t *) sp) = sp + 4;
    sp -= 4;
    *((uint32_t *) sp) = argc;

    uint32_t e = ELF::load(file);
    file == nullptr;

    switchToUser(e,sp,0);
    Debug::panic("*** implement switchToUser");
    return -1;
}

class myFile : public File {
    Atomic<uint32_t> ref_count;
    Shared<Node> node;
    off_t offset;
public:
    myFile(Shared<Node> node, off_t offset): ref_count(0) {
        this->node = node;
        this->offset = offset;
    }
    virtual bool isNotMine() { 
        return false;
    }

    virtual bool isFile() {
        return node->is_file();
    }
    virtual bool isDirectory() {
        return node->is_dir();
    }
    virtual off_t size() {
        return node->size_in_bytes();
    }
    virtual off_t seek(off_t offset) {
        this->offset = offset;
        return this->offset;
    }
    virtual ssize_t read(void* buf, size_t size) {
        off_t reading = node->read_all(offset, size, (char*) buf);
        offset += reading;
        return reading;
    }
    virtual ssize_t write(void* buf, size_t size) {
        return 0;
    }

    friend class Shared<File>;
};

extern "C" int sysHandler(uint32_t eax, uint32_t *frame) {
    using namespace gheith;

    uint32_t *userEsp = (uint32_t*)frame[3];
    uint32_t userPC = frame[0];

    //Debug::printf("*** syscall #%d\n",eax);

    switch (eax) {
    case 0:
        {
            auto status = userEsp[1];
	    //MISSING();
            current()->process->output->set(status);
            stop();
        }
        return 0;
    case 1: /* write */
        {
            int fd = (int) userEsp[1];
            if(fd < 0 || fd > 10) return -1;
            char* buf = (char*) userEsp[2];
            size_t nbyte = (size_t) userEsp[3];
	    //MISSING();
            auto file = current()->process->getFile(fd);
            if (file == nullptr) return -1;
            if(!(file->isNotMine()) || (char*) 0x80000000 > buf || buf > (char*) 0xFFFFFFFF) return -1;
            return file->write(buf,nbyte);
        }
    case 2: /* fork */
    	{
            int id = userEsp[1];
            auto thefork = gheith::current()->process->fork(id);
		    if(thefork == nullptr) return -1;
            gheith::current()->process->output = thefork->output;
            thread(thefork, [&userEsp, &userPC] {
                switchToUser(userPC, (uint32_t) userEsp, 0);
            });
            gheith::current()->process->output->get();
    		return id;
    	}
    case 3: /* sem */
        {
            int initial = (int) userEsp[1];
		    return gheith::current()->process->newSemaphore(initial);
        }

    case 4: /* up */
    	{
		    int id = (int) userEsp[1];
            auto sem = gheith::current()->process->getSemaphore(id); // semaphore not being gotten correctly
    		if(sem == nullptr) return -1;
            sem->up();
            return 0;
    	}
    case 5: /* down */
      	{
		    int id = (int) userEsp[1];
            auto sem = gheith::current()->process->getSemaphore(id);
    		if(sem == nullptr) return -1;
            sem->down();
            return 0;
       	}
    case 6: /* close */
        {
        int id = (int) userEsp[1];
        if(id < 0) return -1;
        return gheith::current()->process->close(id);;
        }

    case 7: /* shutdown */
		Debug::shutdown();
        return -1;

    case 8: /* wait */
        {
        int id = (int) userEsp[1];
        Debug::printf("id: %x\n", id);
        uint32_t* status = (uint32_t*) userEsp[2];
        Debug::printf("status: %x\n", status);
        return gheith::current()->process->wait(id, status);
        }

    case 9: /* execl */
        {
            const char* path = (const char*) userEsp[1];
            if((char*) 0x80000000 > path || path > (char*) 0xFFFFFFFF) return -1;
            int size = 0;
            int idx = 2;
            while(userEsp[idx] != 0) {
                size++;
                idx++;
            }
            const char* args[size];
            for(int i = 0; i < size; i++) {
                args[i] = (const char*) userEsp[i + 2];
                if((char*) 0x80000000 > args[i] || args[i] > (char*) 0xFFFFFFFF) return -1;
            }
            return SYS::exec(path, size, args);
        }

    case 10: /* open */
        {
        const char* fn = (const char*) userEsp[1];
        if((char*) 0x80000000 > fn || fn > (char*) 0xFFFFFFFF) return -1;
        auto thenode = (Shared<Node>) root_fs->find(root_fs->root, fn);
        if(thenode == nullptr) return -1;
        if(thenode->is_symlink()) {
            // check if symlink points to symlink and on and on, if the chain is more than 10 links, return -1
            char* buffer = new char[thenode->size_in_bytes() + 1];
            thenode->get_symbol(buffer);
            buffer[thenode->size_in_bytes()] = 0;
            thenode = (Shared<Node>) root_fs->find(root_fs->root, buffer); // should I use buffer to make node?
            uint32_t counter = 1;
            // pagefaulting on the is_symlink()
            if(thenode == nullptr) return -1;
            while(thenode->is_symlink()) {
                buffer = new char[thenode->size_in_bytes() + 1];
                buffer[thenode->size_in_bytes()] = 0;
                thenode->get_symbol(buffer);
                thenode = (Shared<Node>) root_fs->find(root_fs->root, buffer);
                if(thenode == nullptr) return -1;
                counter++;
                if(counter >= 10) return -1;
            }
        }
        auto thefile = (Shared<File>) new myFile(thenode, 0);

        return gheith::current()->process->setFile(thefile);
        }

    case 11: /* len */
        {
        int fd = (int) userEsp[1];
        if(fd <= 2) return 0;
        auto thefile = gheith::current()->process->getFile(fd);
        if(thefile == nullptr) return -1;
        return thefile->size();
        }

    case 12: /* read */
        {
        int fd = (int) userEsp[1];
        void* buf = (void*) userEsp[2];
        size_t nbyte = (size_t) userEsp[3];
        if(fd <= 2 || buf < (char*) 0x80000000 || buf > (char*) 0xFFFFFFFF) return -1;
        auto thefile = gheith::current()->process->getFile(fd);
        if(thefile != nullptr) return thefile->read(buf, nbyte);
        return -1;
        }

    case 13: /* seek */
        {
        int fd = (int) userEsp[1];
        off_t offset = (off_t) userEsp[2];
        if(fd <= 2) return -1;
        auto thefile = gheith::current()->process->getFile(fd);
        if(thefile == nullptr) return -1;
        return thefile->seek(offset);
        }

    default:
        Debug::printf("*** 1000000000 unknown system call %d\n",eax);
        return -1;
    }
    
}   

void SYS::init(void) {
    IDT::trap(48,(uint32_t)sysHandler_,3);
}
