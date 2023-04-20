#include "stdint.h"
#include "debug.h"
#include "ide.h"
#include "ext2.h"
#include "sys.h"
#include "threads.h"
#include "ps2.h"

const char* initName = "/sbin/init";


namespace gheith {
    Shared<Ext2> root_fs = Shared<Ext2>::make(Shared<Ide>::make(1));
}

void kernelMain(void) {
    auto argv = new const char* [2];
    argv[0] = "init";
    argv[1] = nullptr;
    auto mouse_proc = Shared<Process>::make(true);
    thread(mouse_proc,[]{
        PS2Controller ps2;
        while(true){
            ps2.update();
        }
    });
    
    int rc = SYS::exec(initName,1,argv);
    Debug::panic("*** rc = %d",rc);
}

