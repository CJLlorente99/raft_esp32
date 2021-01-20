#ifndef UV_H
#define UV_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

// Enums definition

typedef enum uv_handle_type {
  UV_UNKNOWN_HANDLE = 0,
  UV_ASYNC,
  UV_CHECK,
  UV_FS_EVENT,
  UV_FS_POLL,
  UV_HANDLE,
  UV_IDLE,
  UV_NAMED_PIPE,
  UV_POLL,
  UV_PREPARE,
  UV_PROCESS,
  UV_STREAM,
  UV_TCP,
  UV_TIMER,
  UV_TTY,
  UV_UDP,
  UV_SIGNAL,
  UV_FILE,
  UV_HANDLE_TYPE_MAX
} uv_handle_type;


// Callbacks declaration and definition

typedef void (*uv__io_cb)(struct uv_loop_s* loop,
                          struct uv__io_s* w,
                          unsigned int events);

typedef void (*uv_close_cb)(uv_handle_t* handle);

typedef void (*uv_signal_cb)(uv_signal_t* handle, int signum);

// Types definition

typedef struct uv_handle_t {
  /* public */                                                                
  void* data;                                                              
  /* read-only */                                                             
  uv_loop_t* loop;                                                            
  uv_handle_type type;                                                        
  /* private */                                                               
  uv_close_cb close_cb;                                                       
  void* handle_queue[2];                                                      
  union {                                                                     
    int fd;                                                                   
    void* reserved[4];                                                        
  } u;                                                                        
  uv_handle_t* next_closing;
  unsigned int flags;
} uv_handle_t;


typedef struct uv__io_t {
  uv__io_cb cb;
  void* pending_queue[2];
  void* watcher_queue[2];
  unsigned int pevents; /* Pending event mask i.e. mask at next tick. */
  unsigned int events;  /* Current event mask. */
  int fd;
  int rcount;
  int wcount;
} uv__io_t;

typedef struct uv_signal_t {
  /* public */                                                                
  void* data;                                                              
  /* read-only */                                                             
  uv_loop_t* loop;                                                            
  uv_handle_type type;                                                        
  /* private */                                                               
  uv_close_cb close_cb;                                                       
  void* handle_queue[2];                                                      
  union {                                                                     
    int fd;                                                                   
    void* reserved[4];                                                        
  } u;                                                                        
  uv_handle_t* next_closing;
  unsigned int flags; 
  uv_signal_cb signal_cb;
  int signum;
  struct {                                                                    
    struct uv_signal_s* rbe_left;                                             
    struct uv_signal_s* rbe_right;                                            
    struct uv_signal_s* rbe_parent;                                           
    int rbe_color;                                                            
  } tree_entry;                                                               
  /* Use two counters here so we don have to fiddle with atomics. */          
  unsigned int caught_signals;                                                
  unsigned int dispatched_signals;
} uv_signal_t;

typedef struct uv_loop_t {
  /* User data - use this for whatever. */
  void* data;
  /* Loop reference counting. */
  unsigned int active_handles;
  void* handle_queue[2];
  union {
    void* unused;
    unsigned int count;
  } active_reqs;
  /* Internal storage for future extensions. */
  void* internal_fields;
  /* Internal flag to signal loop stop. */
  unsigned int stop_flag;

  unsigned long flags;                                                        
  int backend_fd;                                                            
  void* pending_queue[2];                                                    
  void* watcher_queue[2];                                                    
  uv__io_t** watchers;                                                       
  unsigned int nwatchers;                                                    
  unsigned int nfds;                                                         
  void* wq[2];                                                               
  // uv_mutex_t wq_mutex; TODO  uv_mutex_t === pthread_mutex_t                                                      
  // uv_async_t wq_async;                                                       
  // uv_rwlock_t cloexec_lock;                                                  
  // uv_handle_t* closing_handles;                                              
  void* process_handles[2];                                                  
  void* prepare_handles[2];                                                  
  void* check_handles[2];                                                    
  void* idle_handles[2];                                                     
  void* async_handles[2];                                                    
  void (*async_unused)(void);   
  uv__io_t async_io_watcher;                                                 
  int async_wfd;                                                             
  struct timer_heap {                                                                   
    void* min;                                                               
    unsigned int nelts;                                                      
  } timer_heap;                                                              
  uint64_t timer_counter;                                                    
  uint64_t time;                                                             
  int signal_pipefd[2];                                                      
  uv__io_t signal_io_watcher;                                                
  uv_signal_t child_watcher;                                                 
  int emfile_fd;                                                             
  int fs_fs;                                                    
} uv_loop_t;

#endif /* UV_H */
