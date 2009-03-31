/*
  Copyright (c) 2006-2009 Z RESEARCH, Inc. <http://www.zresearch.com>
  This file is part of GlusterFS.

  GlusterFS is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 3 of the License,
  or (at your option) any later version.

  GlusterFS is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/

#ifndef _CONFIG_H
#define _CONFIG_H
#include "config.h"
#endif

#include "call-stub.h"
#include "glusterfs.h"
#include "logging.h"
#include "dict.h"
#include "xlator.h"
#include "io-threads.h"

static void
iot_queue (iot_worker_t *worker,
           call_stub_t *stub);

static call_stub_t *
iot_dequeue (iot_worker_t *worker);

static void
iot_schedule (iot_conf_t *conf,
                inode_t *inode,
                call_stub_t *stub)
{
        int32_t         idx = 0;
        iot_worker_t    *selected_worker = NULL;

        if (inode)
                idx = (inode->ino % conf->thread_count);
        selected_worker = conf->workers[idx];

        iot_queue (selected_worker, stub);
}

int32_t
iot_open_cbk (call_frame_t *frame,
              void *cookie,
              xlator_t *this,
              int32_t op_ret,
              int32_t op_errno,
              fd_t *fd)
{
	STACK_UNWIND (frame, op_ret, op_errno, fd);
	return 0;
}

static int32_t
iot_open_wrapper (call_frame_t * frame,
                xlator_t * this,
                loc_t *loc,
                int32_t flags,
                fd_t * fd)
{
	STACK_WIND (frame, iot_open_cbk, FIRST_CHILD (this),
			FIRST_CHILD (this)->fops->open, loc, flags, fd);
	return 0;
}

int32_t
iot_open (call_frame_t *frame,
          xlator_t *this,
          loc_t *loc,
          int32_t flags,
	  fd_t *fd)
{
        call_stub_t	*stub = NULL;

        stub = fop_open_stub (frame, iot_open_wrapper, loc, flags, fd);
        if (!stub) {
                gf_log (this->name, GF_LOG_ERROR,
                                "cannot get open call stub");
                STACK_UNWIND (frame, -1, ENOMEM, NULL, 0);
                return 0;
        }
	iot_schedule ((iot_conf_t *)this->private, loc->inode, stub);

	return 0;
}


int32_t
iot_create_cbk (call_frame_t *frame,
		void *cookie,
		xlator_t *this,
		int32_t op_ret,
		int32_t op_errno,
		fd_t *fd,
		inode_t *inode,
		struct stat *stbuf)
{
	STACK_UNWIND (frame, op_ret, op_errno, fd, inode, stbuf);
	return 0;
}

int32_t
iot_create_wrapper (call_frame_t *frame,
            xlator_t *this,
	    loc_t *loc,
            int32_t flags,
            mode_t mode,
	    fd_t *fd)
{
	STACK_WIND (frame,
		    iot_create_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->create,
		    loc,
		    flags,
		    mode,
		    fd);
	return 0;
}

int32_t
iot_create (call_frame_t *frame,
            xlator_t *this,
            loc_t *loc,
            int32_t flags,
            mode_t mode,
            fd_t *fd)
{
        call_stub_t     *stub = NULL;

        stub = fop_create_stub (frame, iot_create_wrapper, loc, flags, mode,
                        fd);
        if (!stub) {
                gf_log (this->name, GF_LOG_ERROR,
                                "cannot get create call stub");
                STACK_UNWIND (frame, -1, ENOMEM, NULL, 0);
                return 0;
        }
        iot_schedule ((iot_conf_t *)this->private, loc->inode, stub);
        return 0;
}

int32_t
iot_readv_cbk (call_frame_t *frame,
               void *cookie,
               xlator_t *this,
               int32_t op_ret,
               int32_t op_errno,
               struct iovec *vector,
               int32_t count,
	       struct stat *stbuf)
{
	STACK_UNWIND (frame, op_ret, op_errno, vector, count, stbuf);

	return 0;
}

static int32_t
iot_readv_wrapper (call_frame_t *frame,
                   xlator_t *this,
                   fd_t *fd,
                   size_t size,
                   off_t offset)
{
	STACK_WIND (frame,
		    iot_readv_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->readv,
		    fd,
		    size,
		    offset);
	return 0;
}

int32_t
iot_readv (call_frame_t *frame,
           xlator_t *this,
           fd_t *fd,
           size_t size,
           off_t offset)
{
	call_stub_t *stub;
	stub = fop_readv_stub (frame, 
			       iot_readv_wrapper,
			       fd,
			       size,
			       offset);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, 
			"cannot get readv call stub");
		STACK_UNWIND (frame, -1, ENOMEM, NULL, 0);
		return 0;
	}

        iot_schedule ((iot_conf_t *)this->private, fd->inode, stub);
	return 0;
}

int32_t
iot_flush_cbk (call_frame_t *frame,
               void *cookie,
               xlator_t *this,
               int32_t op_ret,
               int32_t op_errno)
{
	STACK_UNWIND (frame, op_ret, op_errno);
	return 0;
}

static int32_t
iot_flush_wrapper (call_frame_t *frame,
                   xlator_t *this,
                   fd_t *fd)
{
	STACK_WIND (frame,
		    iot_flush_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->flush,
		    fd);
	return 0;
}

int32_t
iot_flush (call_frame_t *frame,
           xlator_t *this,
           fd_t *fd)
{
	call_stub_t *stub;
	stub = fop_flush_stub (frame,
			       iot_flush_wrapper,
			       fd);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get flush_cbk call stub");
		STACK_UNWIND (frame, -1, ENOMEM);
		return 0;
	}

        iot_schedule ((iot_conf_t *)this->private, fd->inode, stub);
	return 0;
}

int32_t
iot_fsync_cbk (call_frame_t *frame,
               void *cookie,
               xlator_t *this,
               int32_t op_ret,
               int32_t op_errno)
{
	STACK_UNWIND (frame, op_ret, op_errno);
	return 0;
}

static int32_t
iot_fsync_wrapper (call_frame_t *frame,
                   xlator_t *this,
                   fd_t *fd,
                   int32_t datasync)
{
	STACK_WIND (frame,
		    iot_fsync_cbk,
		    FIRST_CHILD (this),
		    FIRST_CHILD (this)->fops->fsync,
		    fd,
		    datasync);
	return 0;
}

int32_t
iot_fsync (call_frame_t *frame,
           xlator_t *this,
           fd_t *fd,
           int32_t datasync)
{
	call_stub_t *stub;
	stub = fop_fsync_stub (frame,
			       iot_fsync_wrapper,
			       fd,
			       datasync);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get fsync_cbk call stub");
		STACK_UNWIND (frame, -1, ENOMEM);
		return 0;
	}

        iot_schedule ((iot_conf_t *)this->private, fd->inode, stub);
	return 0;
}

int32_t
iot_writev_cbk (call_frame_t *frame,
                void *cookie,
                xlator_t *this,
                int32_t op_ret,
                int32_t op_errno,
		struct stat *stbuf)
{
	STACK_UNWIND (frame, op_ret, op_errno, stbuf);
	return 0;
}

static int32_t
iot_writev_wrapper (call_frame_t *frame,
                    xlator_t *this,
                    fd_t *fd,
                    struct iovec *vector,
                    int32_t count,
                    off_t offset)
{
	STACK_WIND (frame,
		    iot_writev_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->writev,
		    fd,
		    vector,
		    count,
		    offset);
	return 0;
}

int32_t
iot_writev (call_frame_t *frame,
            xlator_t *this,
            fd_t *fd,
            struct iovec *vector,
            int32_t count,
            off_t offset)
{
	call_stub_t *stub;
	stub = fop_writev_stub (frame, iot_writev_wrapper,
				fd, vector, count, offset);

	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get writev call stub");
		STACK_UNWIND (frame, -1, ENOMEM);
		return 0;
	}

        iot_schedule ((iot_conf_t *)this->private, fd->inode, stub);

	return 0;
}


int32_t
iot_lk_cbk (call_frame_t *frame,
            void *cookie,
            xlator_t *this,
            int32_t op_ret,
            int32_t op_errno,
            struct flock *flock)
{
	STACK_UNWIND (frame, op_ret, op_errno, flock);
	return 0;
}


static int32_t
iot_lk_wrapper (call_frame_t *frame,
                xlator_t *this,
                fd_t *fd,
                int32_t cmd,
                struct flock *flock)
{
	STACK_WIND (frame,
		    iot_lk_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->lk,
		    fd,
		    cmd,
		    flock);
	return 0;
}


int32_t
iot_lk (call_frame_t *frame,
	xlator_t *this,
	fd_t *fd,
	int32_t cmd,
	struct flock *flock)
{
	call_stub_t *stub;
	stub = fop_lk_stub (frame, iot_lk_wrapper,
			    fd, cmd, flock);

	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get fop_lk call stub");
		STACK_UNWIND (frame, -1, ENOMEM, NULL);
		return 0;
	}

        iot_schedule ((iot_conf_t *)this->private, fd->inode, stub);
	return 0;
}


int32_t 
iot_stat_cbk (call_frame_t *frame,
              void *cookie,
              xlator_t *this,
              int32_t op_ret,
              int32_t op_errno,
              struct stat *buf)
{
	STACK_UNWIND (frame, op_ret, op_errno, buf);
	return 0;
}


static int32_t 
iot_stat_wrapper (call_frame_t *frame,
                  xlator_t *this,
                  loc_t *loc)
{
	STACK_WIND (frame,
		    iot_stat_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->stat,
		    loc);
	return 0;
}

int32_t 
iot_stat (call_frame_t *frame,
          xlator_t *this,
          loc_t *loc)
{
	call_stub_t *stub;
	fd_t *fd = NULL;

	fd = fd_lookup (loc->inode, frame->root->pid);

	if (fd == NULL) {
		STACK_WIND(frame,
			   iot_stat_cbk,
			   FIRST_CHILD(this),
			   FIRST_CHILD(this)->fops->stat,
			   loc);
		return 0;
	} 
  
	fd_unref (fd);

	stub = fop_stat_stub (frame,
			      iot_stat_wrapper,
			      loc);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get fop_stat call stub");
		STACK_UNWIND (frame, -1, ENOMEM, NULL);
		return 0;
	}
        iot_schedule ((iot_conf_t *)this->private, loc->inode, stub);

	return 0;
}


int32_t 
iot_fstat_cbk (call_frame_t *frame,
               void *cookie,
               xlator_t *this,
               int32_t op_ret,
               int32_t op_errno,
               struct stat *buf)
{
	STACK_UNWIND (frame, op_ret, op_errno, buf);
	return 0;
}

static int32_t 
iot_fstat_wrapper (call_frame_t *frame,
                   xlator_t *this,
                   fd_t *fd)
{
	STACK_WIND (frame,
		    iot_fstat_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->fstat,
		    fd);
	return 0;
}

int32_t 
iot_fstat (call_frame_t *frame,
           xlator_t *this,
           fd_t *fd)
{
	call_stub_t *stub;
	stub = fop_fstat_stub (frame,
			       iot_fstat_wrapper,
			       fd);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get fop_fstat call stub");
		STACK_UNWIND (frame, -1, ENOMEM, NULL);
		return 0;
	}

        iot_schedule ((iot_conf_t *)this->private, fd->inode, stub);

	return 0;
}

int32_t 
iot_truncate_cbk (call_frame_t *frame,
                  void *cookie,
                  xlator_t *this,
                  int32_t op_ret,
                  int32_t op_errno,
                  struct stat *buf)
{
	STACK_UNWIND (frame, op_ret, op_errno, buf);
	return 0;
}

static int32_t 
iot_truncate_wrapper (call_frame_t *frame,
                      xlator_t *this,
                      loc_t *loc,
                      off_t offset)
{
	STACK_WIND (frame,
		    iot_truncate_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->truncate,
		    loc,
		    offset);
	return 0;
}

int32_t 
iot_truncate (call_frame_t *frame,
              xlator_t *this,
              loc_t *loc,
              off_t offset)
{
	call_stub_t *stub;
	fd_t *fd = NULL;
  
	fd = fd_lookup (loc->inode, frame->root->pid);

	if (fd == NULL) {
		STACK_WIND(frame,
			   iot_truncate_cbk,
			   FIRST_CHILD(this),
			   FIRST_CHILD(this)->fops->truncate,
			   loc,
			   offset);
		return 0;
	} 
  
	fd_unref (fd);

	stub = fop_truncate_stub (frame,
				  iot_truncate_wrapper,
				  loc,
				  offset);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get fop_stat call stub");
		STACK_UNWIND (frame, -1, ENOMEM, NULL);
		return 0;
	}
        iot_schedule ((iot_conf_t *)this->private, loc->inode, stub);

	return 0;
}

int32_t 
iot_ftruncate_cbk (call_frame_t *frame,
                   void *cookie,
                   xlator_t *this,
                   int32_t op_ret,
                   int32_t op_errno,
                   struct stat *buf)
{
	STACK_UNWIND (frame, op_ret, op_errno, buf);
	return 0;
}

static int32_t 
iot_ftruncate_wrapper (call_frame_t *frame,
                       xlator_t *this,
                       fd_t *fd,
                       off_t offset)
{
	STACK_WIND (frame,
		    iot_ftruncate_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->ftruncate,
		    fd,
		    offset);
	return 0;
}

int32_t 
iot_ftruncate (call_frame_t *frame,
               xlator_t *this,
               fd_t *fd,
               off_t offset)
{
	call_stub_t *stub;
	stub = fop_ftruncate_stub (frame,
				   iot_ftruncate_wrapper,
				   fd,
				   offset);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get fop_ftruncate call stub");
		STACK_UNWIND (frame, -1, ENOMEM, NULL);
		return 0;
	}
        iot_schedule ((iot_conf_t *)this->private, fd->inode, stub);

	return 0;
}

int32_t 
iot_utimens_cbk (call_frame_t *frame,
                 void *cookie,
                 xlator_t *this,
                 int32_t op_ret,
                 int32_t op_errno,
                 struct stat *buf)
{
	STACK_UNWIND (frame, op_ret, op_errno, buf);
	return 0;
}

static int32_t 
iot_utimens_wrapper (call_frame_t *frame,
                     xlator_t *this,
                     loc_t *loc,
                     struct timespec tv[2])
{
	STACK_WIND (frame,
		    iot_utimens_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->utimens,
		    loc,
		    tv);
  
	return 0;
}

int32_t 
iot_utimens (call_frame_t *frame,
             xlator_t *this,
             loc_t *loc,
             struct timespec tv[2])
{
	call_stub_t *stub;
	fd_t *fd = NULL;
  
	fd = fd_lookup (loc->inode, frame->root->pid);

	if (fd == NULL) {
		STACK_WIND(frame,
			   iot_utimens_cbk,
			   FIRST_CHILD(this),
			   FIRST_CHILD(this)->fops->utimens,
			   loc,
			   tv);
		return 0;
	} 
  
	fd_unref (fd);

	stub = fop_utimens_stub (frame,
				 iot_utimens_wrapper,
				 loc,
				 tv);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get fop_utimens call stub");
		STACK_UNWIND (frame, -1, ENOMEM, NULL);
		return 0;
	}
        iot_schedule ((iot_conf_t *)this->private, loc->inode, stub);

	return 0;
}


int32_t 
iot_checksum_cbk (call_frame_t *frame,
		  void *cookie,
		  xlator_t *this,
		  int32_t op_ret,
		  int32_t op_errno,
		  uint8_t *file_checksum,
		  uint8_t *dir_checksum)
{
	STACK_UNWIND (frame, op_ret, op_errno, file_checksum, dir_checksum);
	return 0;
}

static int32_t 
iot_checksum_wrapper (call_frame_t *frame,
		      xlator_t *this,
		      loc_t *loc,
		      int32_t flags)
{
	STACK_WIND (frame,
		    iot_checksum_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->checksum,
		    loc,
		    flags);
  
	return 0;
}

int32_t 
iot_checksum (call_frame_t *frame,
	      xlator_t *this,
	      loc_t *loc,
	      int32_t flags)
{
	call_stub_t *stub = NULL;

	stub = fop_checksum_stub (frame,
				  iot_checksum_wrapper,
				  loc,
				  flags);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get fop_checksum call stub");
		STACK_UNWIND (frame, -1, ENOMEM, NULL, NULL);
		return 0;
	}
        iot_schedule ((iot_conf_t *)this->private, loc->inode, stub);

	return 0;
}


int32_t 
iot_unlink_cbk (call_frame_t *frame,
		void *cookie,
		xlator_t *this,
		int32_t op_ret,
		int32_t op_errno)
{
	STACK_UNWIND (frame, op_ret, op_errno);
	return 0;
}

static int32_t 
iot_unlink_wrapper (call_frame_t *frame,
		    xlator_t *this,
		    loc_t *loc)
{
	STACK_WIND (frame,
		    iot_unlink_cbk,
		    FIRST_CHILD(this),
		    FIRST_CHILD(this)->fops->unlink,
		    loc);
  
	return 0;
}

int32_t 
iot_unlink (call_frame_t *frame,
	    xlator_t *this,
	    loc_t *loc)
{
	call_stub_t *stub = NULL;
	stub = fop_unlink_stub (frame, iot_unlink_wrapper, loc);
	if (!stub) {
		gf_log (this->name, GF_LOG_ERROR, "cannot get fop_unlink call stub");
		STACK_UNWIND (frame, -1, ENOMEM);
		return 0;
	}
        iot_schedule ((iot_conf_t *)this->private, loc->inode, stub);

	return 0;
}


static void
iot_queue (iot_worker_t *worker,
           call_stub_t *stub)
{
	iot_request_t   *req = NULL;

        req = CALLOC (1, sizeof (iot_request_t));
        ERR_ABORT (req);
        req->stub = stub;

        pthread_mutex_lock (&worker->qlock);
        {
                list_add_tail (&req->list, &worker->rqlist);

                /* dq_cond */
                worker->queue_size++;
                pthread_cond_broadcast (&worker->dq_cond);
        }
	pthread_mutex_unlock (&worker->qlock);
}

static call_stub_t *
iot_dequeue (iot_worker_t *worker)
{
	call_stub_t *stub = NULL;
	iot_request_t *req = NULL;

	pthread_mutex_lock (&worker->qlock);
        {
                while (!worker->queue_size)
                        pthread_cond_wait (&worker->dq_cond, &worker->qlock);

                list_for_each_entry (req, &worker->rqlist, list)
                        break;
                list_del (&req->list);
                stub = req->stub;

                worker->queue_size--;
        }
	pthread_mutex_unlock (&worker->qlock);

	FREE (req);

	return stub;
}

static void *
iot_worker (void *arg)
{
	iot_worker_t *worker = arg;

	while (1) {
		call_stub_t *stub;

		stub = iot_dequeue (worker);
		call_resume (stub);
	}
}

static iot_worker_t **
allocate_worker_array (int count)
{
        iot_worker_t    ** warr = NULL;

        warr = CALLOC (count, sizeof(iot_worker_t *));
        ERR_ABORT (warr);

        return warr;
}

static iot_worker_t *
allocate_worker (iot_conf_t * conf)
{
        iot_worker_t    *wrk = NULL;

        wrk = CALLOC (1, sizeof (iot_worker_t));
        ERR_ABORT (wrk);

        INIT_LIST_HEAD (&wrk->rqlist);
        wrk->conf = conf;
        pthread_cond_init (&wrk->dq_cond, NULL);
        pthread_mutex_init (&wrk->qlock, NULL);

        return wrk;
}

static void
allocate_workers (iot_conf_t *conf,
                int count,
                int start_alloc_idx)
{
        int     i, end_count;

        end_count = count + start_alloc_idx;
        for (i = start_alloc_idx; i < end_count; i++) {
                conf->workers[i] = allocate_worker (conf);
                pthread_create (&conf->workers[i]->thread, NULL, iot_worker,
                                conf->workers[i]);
        }
}

static void
workers_init (iot_conf_t *conf)
{
        conf->workers = allocate_worker_array (conf->thread_count);
        allocate_workers (conf, conf->thread_count, 0);
}



int32_t 
init (xlator_t *this)
{
	iot_conf_t *conf;
	dict_t *options = this->options;

	if (!this->children || this->children->next) {
		gf_log ("io-threads",
			GF_LOG_ERROR,
			"FATAL: iot not configured with exactly one child");
		return -1;
	}

	if (!this->parents) {
		gf_log (this->name, GF_LOG_WARNING,
			"dangling volume. check volfile ");
	}

	conf = (void *) CALLOC (1, sizeof (*conf));
	ERR_ABORT (conf);

	conf->thread_count = 1;

	if (dict_get (options, "thread-count")) {
		conf->thread_count = data_to_int32 (dict_get (options,
							      "thread-count"));
		gf_log ("io-threads",
			GF_LOG_DEBUG,
			"Using conf->thread_count = %d",
			conf->thread_count);
	}

	workers_init (conf);

	this->private = conf;
	return 0;
}

void
fini (xlator_t *this)
{
	iot_conf_t *conf = this->private;

	FREE (conf);

	this->private = NULL;
	return;
}

struct xlator_fops fops = {
	.open        = iot_open,
	.create      = iot_create,
	.readv       = iot_readv,
	.writev      = iot_writev,
	.flush       = iot_flush,
	.fsync       = iot_fsync,
	.lk          = iot_lk,
	.stat        = iot_stat,
	.fstat       = iot_fstat,
	.truncate    = iot_truncate,
	.ftruncate   = iot_ftruncate,
	.utimens     = iot_utimens,
	.checksum    = iot_checksum,
	.unlink      = iot_unlink,
};

struct xlator_mops mops = {
};

struct xlator_cbks cbks = {
};

struct volume_options options[] = {
	{ .key  = {"thread-count"}, 
	  .type = GF_OPTION_TYPE_INT, 
	  .min  = 1, 
	  .max  = 32
	},
	{ .key  = {NULL} },
};
