/*
 * data_am.c
 *
 *  Created on: Dec 7, 2015
 *      Author: xiaoxin
 */

/*
 * interface for data access method.
 */

#include<pthread.h>
#include<assert.h>
#include<stdbool.h>
#include<sys/socket.h>
#include<assert.h>
#include"config.h"
#include"data_am.h"
#include"data_record.h"
#include"lock_record.h"
#include"thread_global.h"
#include"proc.h"
#include"trans.h"
#include"transactions.h"
#include"data.h"
#include"socket.h"
#include"communicate.h"

/*
 * @return: '0' to rollback, '1' to go head.
 */
int Data_Insert(int table_id, TupleId tuple_id, TupleId value, int nid)
{
    int index;
    int status;
    DataRecord datard;
    THREAD* threadinfo;

    uint64_t* sbuffer;
    uint64_t* rbuffer;
    int conn;

    /* get the pointer to current thread information. */
    threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
    index=threadinfo->index;

    /*
     * the node transaction process must to get the data from the storage process in the
     * node itself or in the other nodes, both use the socket to communicate.
     */

    int lindex;
    lindex = GetLocalIndex(index);

    sbuffer=send_buffer[lindex];
    rbuffer=recv_buffer[lindex];
    conn=connect_socket[nid][lindex];

    //send data-insert to node "nid".
    *(sbuffer) = cmd_insert;
    *(sbuffer+1) = table_id;
    *(sbuffer+2) = tuple_id;
    *(sbuffer+3) = value;

    int num = 4;
    Send(conn, sbuffer, num);

    // response from "nid".
    num = 1;
    Receive(conn, rbuffer, num);

    status = *(rbuffer);


    if (status == 0)
        return 0;

    datard.node_id = nid;
    DataRecordInsert(&datard);

    return 1;
}

/*
 * @return:'0' for not found, '1' for success, '-1' for update-conflict-rollback.
 */
int Data_Update(int table_id, TupleId tuple_id, TupleId value, int nid)
{
    int index=0;
    DataRecord datard;

    int status;
    THREAD* threadinfo;

    bool isdelete = false;

    uint64_t* sbuffer;
    uint64_t* rbuffer;
    int conn;

    /* get the pointer to current thread information. */
    threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
    index=threadinfo->index;
    int lindex;
    lindex = GetLocalIndex(index);

    sbuffer=send_buffer[lindex];
    rbuffer=recv_buffer[lindex];
    conn=connect_socket[nid][lindex];

    //send data-insert to node "nid".
    *(sbuffer) = cmd_update;
    *(sbuffer+1) = table_id;
    *(sbuffer+2) = tuple_id;
    *(sbuffer+3) = value;
    *(sbuffer+4) = isdelete;

    int num = 5;
    Send(conn, sbuffer, num);

    // response from "nid".
    num = 1;
    Receive(conn, rbuffer, num);

    status = *(rbuffer);

    if (status == 0)
        return 0;
    /* record the updated data. */
    datard.node_id = nid;
    DataRecordInsert(&datard);
    return 1;
}

/*
 * @return:'0' for not found, '1' for success, '-1' for update-conflict-rollback.
 */

int Data_Delete(int table_id, TupleId tuple_id, int nid)
{
    int index=0;
    DataRecord datard;
    uint64_t value = InvalidTupleId;
    int status;
    THREAD* threadinfo;

    bool isdelete = true;

    uint64_t* sbuffer;
    uint64_t* rbuffer;
    int conn;

    /* get the pointer to current thread information. */
    threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
    index=threadinfo->index;
    int lindex;
    lindex = GetLocalIndex(index);

    sbuffer=send_buffer[lindex];
    rbuffer=recv_buffer[lindex];
    conn=connect_socket[nid][lindex];

    //send data-insert to node "nid".
    *(sbuffer) = cmd_update;
    *(sbuffer+1) = table_id;
    *(sbuffer+2) = tuple_id;
    *(sbuffer+3) = value;
    *(sbuffer+4) = isdelete;

    int num = 5;
    Send(conn, sbuffer, num);

    // response from "nid".
    num = 1;
    Receive(conn, rbuffer, num);

    status = *(rbuffer);


    if (status == 0)
        return 0;

    /* record the updated data. */
    datard.node_id = nid;
    DataRecordInsert(&datard);
    return 1;
}

/*
 * @input:'isupdate':true for reading before updating, false for commonly reading.
 * @return:NULL for read nothing, to rollback or just let it go.
 */
TupleId Data_Read(int table_id, TupleId tuple_id, int nid, int* flag)
{
    int index;
    uint64_t value;
    TransactionData* tdata;
    THREAD* threadinfo;
    *flag=1;

    uint64_t* sbuffer;
    uint64_t* rbuffer;
    int conn;

    /* get the pointer to current thread information. */
    threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
    tdata=(TransactionData*)pthread_getspecific(TransactionDataKey);

    index=threadinfo->index;
    int lindex;
    lindex = GetLocalIndex(index);

    sbuffer=send_buffer[lindex];
    rbuffer=recv_buffer[lindex];
    conn=connect_socket[nid][lindex];

    //send data-insert to node "nid".
    *(sbuffer) = cmd_read;
    *(sbuffer+1) = table_id;
    *(sbuffer+2) = tuple_id;

    int num = 3;
    Send(conn, sbuffer, num);

    // response from "nid".
    num = 2;
    Receive(conn, rbuffer, num);

    *flag = *(rbuffer);
    value = *(rbuffer+1);

    *flag = *(recv_buffer[lindex]);
    value = *(recv_buffer[lindex]+1);


    if (*flag != 1)
    {
        return 0;
    }

    else
    {
        return value;
    }

    return 0;
}
