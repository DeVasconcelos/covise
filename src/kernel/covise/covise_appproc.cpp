/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include "covise.h"
#include "covise_appproc.h"
#include <shm/covise_shm.h>
#include <net/covise_socket.h>
#include <net/covise_host.h>
#include <messages/CRB_EXEC.h>

#ifdef _WIN32
typedef int pid_t;
#endif

using namespace covise;

ApplicationProcess *ApplicationProcess::approc = NULL;

/***********************************************************************\ 
 **                                                                     **
 **   ApplicationProcess class Routines            Version: 1.1         **
 **                                                                     **
 **                                                                     **
 **   Description  : The ApplicationProcess handles the infrastructure  **
 **                  for the environment                                **
 **                                                                     **
 **   Classes      : ApplicationProcess                                 **
 **                                                                     **
 **   Copyright (C) 1993     by University of Stuttgart                 **
 **                             Computer Center (RUS)                   **
 **                             Allmandring 30                          **
 **                             7000 Stuttgart 80                       **
 **                                                                     **
 **                                                                     **
 **   Author       : A. Wierse   (RUS)                                  **
 **                                                                     **
 **   History      :                                                    **
 **                  15.04.93  Ver 1.0                                  **
 **                  26.05.93  Ver 1.1 now the shared memory key comes  **
 **                                    from the datamanager             **
 **                                                                     **
 **                                                                     **
\***********************************************************************/

ApplicationProcess::~ApplicationProcess()
{
    //	delete datamanager;
    //delete part_obj_list;
    delete shm;
}; // destructor

void *ApplicationProcess::get_shared_memory_address()
{
    return shm->get_pointer();
}

void ApplicationProcess::send_data_msg(Message *msg)
{
    if (!datamanager->sendMessage(msg))
        list_of_connections->remove(datamanager);
}

void ApplicationProcess::recv_data_msg(Message *msg)
{
    datamanager->recv_msg(msg);
    msg->conn = datamanager;
}

void ApplicationProcess::exch_data_msg(Message *msg, const std::vector<int> &messageTypes)
{
    if (!datamanager->sendMessage(msg))
    {
        list_of_connections->remove(datamanager);
        return;
    }
    msg->data = DataHandle{};

    while (datamanager->recv_msg(msg))
    {
        if (std::find(messageTypes.begin(), messageTypes.end(), msg->type) != messageTypes.end() || msg->type == COVISE_MESSAGE_SOCKET_CLOSED)
        {
            return;
        }
        if (msg->type == COVISE_MESSAGE_NEW_SDS)
        {
            handle_shm_msg(msg);
            msg->data = DataHandle{};
        }
        else
        {
            Message *list_msg = new Message;
            list_msg->copyAndReuseData(*msg);
            msg_queue->add(list_msg);
#ifdef DEBUG
        print_comment(__LINE__, __FILE__, (std::string{"msg "} + covise_msg_types_array[msg->type] + " added to queue").c_str());
#endif
        }
    }
}

void ApplicationProcess::contact_datamanager(int p)
{
    //    print_comment(__LINE__, __FILE__, "nach new Connection");
    datamanager = list_of_connections->addNewConn<DataManagerConnection>(p, id, (int)send_type);
   
    Message msg{ COVISE_MESSAGE_GET_SHM_KEY, DataHandle{} };
    exch_data_msg(&msg, {COVISE_MESSAGE_GET_SHM_KEY});
    if (msg.type != COVISE_MESSAGE_GET_SHM_KEY || msg.data.data() == nullptr)
    {
        cerr << "didn't get GET_SHM_KEY\n";
        print_exit(__LINE__, __FILE__, 1);
    }
    delete shm;
    shm = new ShmAccess(msg.data.accessData());
}

int ApplicationProcess::check_msg_queue()
{
    msg_queue->reset();
    return (msg_queue->next() != NULL);
}

Message *ApplicationProcess::wait_for_ctl_msg()
{
    char tmp_str[255];
    Message *msg;
    int end = 0;

    //    print_comment(__LINE__, __FILE__, "in wait_for_ctl_msg");
    while (!end)
    {
        msg = wait_for_msg();
        sprintf(tmp_str, "msg->type %d received", msg->type);
        print_comment(__LINE__, __FILE__, "msg->type %d received", msg->type);
        if (msg->conn == datamanager)
        {
            print_comment(__LINE__, __FILE__, "process_msg_from_dmgr");
            process_msg_from_dmgr(msg);
             msg->data = DataHandle();
            delete msg;
        }
        else
            end = 1;
    }
    print_comment(__LINE__, __FILE__, "return(msg)");
    return (msg);
}

Message *ApplicationProcess::check_for_ctl_msg(float time)
{
    Message *msg;

    // print_comment(__LINE__, __FILE__, "in check_for_ctl_msg");
    msg = check_for_msg(time);
    while ((msg != NULL)
           && (msg->conn == datamanager))
    {
        print_comment(__LINE__, __FILE__, "msg->type %d received", msg->type);
        print_comment(__LINE__, __FILE__, "process_msg_from_dmgr");
        process_msg_from_dmgr(msg);
        msg->data = DataHandle();
        delete msg;
        print_comment(__LINE__, __FILE__, "recheck for msg");
        msg = check_for_msg(time);
    }
    if (msg != NULL)
    {
        print_comment(__LINE__, __FILE__, "msg->type %d received", msg->type);
        print_comment(__LINE__, __FILE__, "return(msg)");
    }
    else
    {
        print_comment(__LINE__, __FILE__, "return NULL");
    }
    return msg;
}

void ApplicationProcess::process_msg_from_dmgr(Message *msg)
{
    switch (msg->type)
    {
    case COVISE_MESSAGE_NEW_SDS:
        handle_shm_msg(msg);
        break;
    default:
        break;
    }
}

static ApplicationProcess *appl_process;

#ifndef _WIN32
#ifdef COVISE_Signals
//=====================================================================
//
//=====================================================================
void appproc_signal_handler(int, void *)
{
    delete appl_process;
    sleep(1);
    exit(EXIT_FAILURE);
}
#endif
#endif

// initialize process
ApplicationProcess::ApplicationProcess(const char *name, int id)
    : OrdinaryProcess(name, id, APPLICATIONMODULE)
{
    approc = appl_process = this;
    datamanager = NULL;
    //part_obj_list = new List<coDistributedObject>;
    shm = NULL;
#ifdef COVISE_Signals
    // Initialization of signal handlers
    sig_handler.addSignal(SIGBUS, (void *)appproc_signal_handler, NULL);
    // SIGPIPE is handled by Process::Process
    //    sig_handler.addSignal(SIGPIPE,(void *)appproc_signal_handler,NULL);
    sig_handler.addSignal(SIGTERM, (void *)appproc_signal_handler, NULL);
    sig_handler.addSignal(SIGSEGV, (void *)appproc_signal_handler, NULL);
    sig_handler.addSignal(SIGFPE, (void *)appproc_signal_handler, NULL);
//    init_emergency_message();
#endif
}

ApplicationProcess::ApplicationProcess(const char *n, int argc, char *argv[],
                                       sender_type send_type)
    : OrdinaryProcess(n, argc, argv, send_type)
{
    //fprintf(stderr,"---- in ApplicationProcess::ApplicationProcess\n");
    //part_obj_list = NULL;
    static Host *tmphost;
    int pid;
    unsigned int uport;
#ifdef DEBUG
    char tmp_str[255];
#endif
    shm = NULL;
    auto crbExec = covise::getExecFromCmdArgs(argc, argv);

    id = crbExec.moduleCount();
    tmphost = new Host(crbExec.controllerIp());

    //fprintf(stderr,"---- contact_controller\n");

    datamanager = NULL;
    contact_controller(crbExec.controllerPort(), tmphost);
    // darf ich das? Uwe WOessner delete tmphost;
    if (!is_connected())
        return;
    approc = appl_process = this;
//part_obj_list = new List<coDistributedObject>;
    Message *msg = wait_for_ctl_msg();
    if (msg->type == COVISE_MESSAGE_APP_CONTACT_DM)
    {
        TokenBuffer tb{msg};
        int p;
        tb >> p;
        uport = p;
        contact_datamanager(p);

        msg->type = COVISE_MESSAGE_SEND_APPL_PROCID;
        pid = getpid();
        msg->data = DataHandle{ (char*)& pid, sizeof(pid_t), false };
#ifdef DEBUG
        sprintf(tmp_str, "SEND_APPL_PROCID: %d", *(pid_t *)msg->data);
        print_comment(__LINE__, __FILE__, tmp_str);
        sprintf(tmp_str, "pid: %d", pid);
        print_comment(__LINE__, __FILE__, tmp_str);
#endif
        if (!datamanager->sendMessage(msg))
        {
            list_of_connections->remove(datamanager);
            print_error(__LINE__, __FILE__,
                        "datamaner socket invalid in new ApplicationProcess");
        }
        //cerr << "contact succeeded\n";
    }
    else
    {
        cerr << "wrong message instead of APP_CONTACT_DM (" << msg->type << ")" << endl;
        cerr << "Did you specify Shared Memory type \"none\" in your covise config?" << endl;
        cerr << "Please, change it to shm." << endl;
        print_exit(__LINE__, __FILE__, 1);
    }
    delete msg;
#ifdef COVISE_Signals
    // Initialization of signal handlers
    sig_handler.addSignal(SIGBUS, (void *)appproc_signal_handler, NULL);
    sig_handler.addSignal(SIGPIPE, (void *)appproc_signal_handler, NULL);
    sig_handler.addSignal(SIGTERM, (void *)appproc_signal_handler, NULL);
    sig_handler.addSignal(SIGSEGV, (void *)appproc_signal_handler, NULL);
    sig_handler.addSignal(SIGFPE, (void *)appproc_signal_handler, NULL);
//    init_emergency_message();
#endif
}

void ApplicationProcess::contact_controller(int p, Host *h)
{
    controller = list_of_connections->addNewConn<ControllerConnection>(h, p, id, (int)send_type);
}

void ApplicationProcess::handle_shm_msg(Message *msg)
{
    int tmpkey, size;

    tmpkey = ((int *)msg->data.data())[0];
    size = ((int *)msg->data.data())[1];
    shm->add_new_segment(tmpkey, size);
    //cerr << "new SharedMemory" << endl;
    print_comment(__LINE__, __FILE__, "new SharedMemory");
}

#if 0
coDistributedObject *ApplicationProcess::get_part_obj(char *pname)
{
   class coDistributedObject *pobj;

   part_obj_list->reset();
   pobj = part_obj_list->next();
   while(pobj)
   {
      if(strcmp(pobj->getName(), pname) == 0)
         return pobj;
   }
   return 0;
}
#endif

void UserInterface::contact_controller(int p, Host *h)
{
    controller = list_of_connections->addNewConn<ControllerConnection>(h, p, id, send_type);
}
