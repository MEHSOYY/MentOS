/// @file msg.h
/// @brief Definition of structure for managing message queues.
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#pragma once

#include "stddef.h"
#include "sys/ipc.h"
#include "sys/types.h"
#include "time.h"

/// The maximum size for a message text.
#define MSGMAX 8192
/// The default maximum size in bytes of a message queue.
#define MSGMNB 16384

/// Specify that we should not treat as an error if the message we are trying to
/// return to the user, is too big. That message will be truncated to fit the
/// user-defined reading buffer. If not specified, the message is kept in the
/// queue, and an error is returned.
#define MSG_NOERROR 010000

/// @brief Buffer to use with the message queue IPC.
struct msgbuf {
    /// Type of the message.
    long mtype;
    /// Text of the message.
    char mtext[1];
};

/// Keeps track of a stored message.
struct msg {
    /// Pointer to the next message on queue.
    struct msg *msg_next;
    /// The type of message.
    long msg_type;
    /// Pointer to the beginning of the message.
    char *msg_ptr;
    /// The length of the message.
    size_t msg_size;
};

/// @brief Message queue data structure.
struct msqid_ds {
    /// Ownership and permissions.
    struct ipc_perm msg_perm;
    /// Time of last msgsnd(2).
    time_t msg_stime;
    /// Time of last msgrcv(2).
    time_t msg_rtime;
    /// Time of creation or last modification by msgctl().
    time_t msg_ctime;
    /// Number of bytes in queue.
    unsigned short msg_cbytes;
    /// Number of messages in queue.
    unsigned short msg_qnum;
    /// Maximum number of bytes in queue.
    unsigned short msg_qbytes;
    /// PID of last msgsnd(2).
    pid_t msg_lspid;
    /// PID of last msgrcv(2).
    pid_t msg_lrpid;
};

/// @brief Get a System V message queue identifier.
/// @param key can be used either to obtain the identifier of a previously
/// created message queue, or to create a new set.
/// @param msgflg controls the behaviour of the function.
/// @return the message queue identifier, -1 on failure, and errno is set to
/// indicate the error.
int msgget(key_t key, int msgflg);

/// @brief Used to send messages.
/// @param msqid the message queue identifier.
/// @param msgp points to a used-defined msgbuf.
/// @param msgsz specifies the size in bytes of mtext.
/// @param msgflg specifies the action to be taken in case of specific events.
/// @return 0 on success, -1 on failure and errno is set to indicate the error.
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);

/// @brief Used to receive messages.
/// @param msqid the message queue identifier.
/// @param msgp points to a used-defined msgbuf.
/// @param msgsz specifies the size in bytes of mtext.
/// @param msgtyp specifies the type of message requested, as follows:
/// - msgtyp == 0: the first message on the queue is received.
/// - msgtyp  > 0: the first message of type, msgtyp, is received.
/// - msgtyp  < 0: the first message of the lowest type that is less than or
///                equal to the absolute value of msgtyp is received.
/// @param msgflg specifies the action to be taken in case of specific events.
/// @return the number of bytes actually copied on success, -1 on failure and
/// errno is set to indicate the error.
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);

/// @brief Message queue control operations.
/// @param msqid the message queue identifier.
/// @param cmd The command to perform.
/// @param buf used with IPC_STAT and IPC_SET.
/// @return 0 on success, -1 on failure and errno is set to indicate the error.
int msgctl(int msqid, int cmd, struct msqid_ds *buf);
