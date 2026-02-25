//
// Copyright 2019 EchoNous Inc.
//

#pragma once

#include <CL/opencl.h>


// Shim layer/loader for opencl functions

bool LoadOpenCL();


typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetPlatformIDs)(cl_uint          /* num_entries */,
                 cl_platform_id * /* platforms */,
                 cl_uint *        /* num_platforms */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetPlatformInfo)(cl_platform_id   /* platform */,
                  cl_platform_info /* param_name */,
                  size_t           /* param_value_size */,
                  void *           /* param_value */,
                  size_t *         /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

/* Device APIs */
typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetDeviceIDs)(cl_platform_id   /* platform */,
               cl_device_type   /* device_type */,
               cl_uint          /* num_entries */,
               cl_device_id *   /* devices */,
               cl_uint *        /* num_devices */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetDeviceInfo)(cl_device_id    /* device */,
                cl_device_info  /* param_name */,
                size_t          /* param_value_size */,
                void *          /* param_value */,
                size_t *        /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clCreateSubDevices)(cl_device_id                         /* in_device */,
                   const cl_device_partition_property * /* properties */,
                   cl_uint                              /* num_devices */,
                   cl_device_id *                       /* out_devices */,
                   cl_uint *                            /* num_devices_ret */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clRetainDevice)(cl_device_id /* device */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clReleaseDevice)(cl_device_id /* device */) CL_API_SUFFIX__VERSION_1_2;

/* Context APIs  */
typedef CL_API_ENTRY cl_context CL_API_CALL
(*PFN_clCreateContext)(const cl_context_properties * /* properties */,
                cl_uint                 /* num_devices */,
                const cl_device_id *    /* devices */,
                void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
                void *                  /* user_data */,
                cl_int *                /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_context CL_API_CALL
(*PFN_clCreateContextFromType)(const cl_context_properties * /* properties */,
                        cl_device_type          /* device_type */,
                        void (CL_CALLBACK *     /* pfn_notify*/ )(const char *, const void *, size_t, void *),
                        void *                  /* user_data */,
                        cl_int *                /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clRetainContext)(cl_context /* context */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clReleaseContext)(cl_context /* context */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetContextInfo)(cl_context         /* context */,
                 cl_context_info    /* param_name */,
                 size_t             /* param_value_size */,
                 void *             /* param_value */,
                 size_t *           /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

/* Command Queue APIs */
typedef CL_API_ENTRY cl_command_queue CL_API_CALL
(*PFN_clCreateCommandQueueWithProperties)(cl_context               /* context */,
                                   cl_device_id             /* device */,
                                   const cl_queue_properties *    /* properties */,
                                   cl_int *                 /* errcode_ret */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clRetainCommandQueue)(cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clReleaseCommandQueue)(cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetCommandQueueInfo)(cl_command_queue      /* command_queue */,
                      cl_command_queue_info /* param_name */,
                      size_t                /* param_value_size */,
                      void *                /* param_value */,
                      size_t *              /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

/* Memory Object APIs */
typedef CL_API_ENTRY cl_mem CL_API_CALL
(*PFN_clCreateBuffer)(cl_context   /* context */,
               cl_mem_flags /* flags */,
               size_t       /* size */,
               void *       /* host_ptr */,
               cl_int *     /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_mem CL_API_CALL
(*PFN_clCreateSubBuffer)(cl_mem                   /* buffer */,
                  cl_mem_flags             /* flags */,
                  cl_buffer_create_type    /* buffer_create_type */,
                  const void *             /* buffer_create_info */,
                  cl_int *                 /* errcode_ret */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_mem CL_API_CALL
(*PFN_clCreateImage)(cl_context              /* context */,
              cl_mem_flags            /* flags */,
              const cl_image_format * /* image_format */,
              const cl_image_desc *   /* image_desc */,
              void *                  /* host_ptr */,
              cl_int *                /* errcode_ret */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_mem CL_API_CALL
(*PFN_clCreatePipe)(cl_context                 /* context */,
             cl_mem_flags               /* flags */,
             cl_uint                    /* pipe_packet_size */,
             cl_uint                    /* pipe_max_packets */,
             const cl_pipe_properties * /* properties */,
             cl_int *                   /* errcode_ret */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clRetainMemObject)(cl_mem /* memobj */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clReleaseMemObject)(cl_mem /* memobj */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetSupportedImageFormats)(cl_context           /* context */,
                           cl_mem_flags         /* flags */,
                           cl_mem_object_type   /* image_type */,
                           cl_uint              /* num_entries */,
                           cl_image_format *    /* image_formats */,
                           cl_uint *            /* num_image_formats */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetMemObjectInfo)(cl_mem           /* memobj */,
                   cl_mem_info      /* param_name */,
                   size_t           /* param_value_size */,
                   void *           /* param_value */,
                   size_t *         /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetImageInfo)(cl_mem           /* image */,
               cl_image_info    /* param_name */,
               size_t           /* param_value_size */,
               void *           /* param_value */,
               size_t *         /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetPipeInfo)(cl_mem           /* pipe */,
              cl_pipe_info     /* param_name */,
              size_t           /* param_value_size */,
              void *           /* param_value */,
              size_t *         /* param_value_size_ret */) CL_API_SUFFIX__VERSION_2_0;


typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clSetMemObjectDestructorCallback)(cl_mem /* memobj */,
                                 void (CL_CALLBACK * /*pfn_notify*/)( cl_mem /* memobj */, void* /*user_data*/),
                                 void * /*user_data */ )             CL_API_SUFFIX__VERSION_1_1;

/* SVM Allocation APIs */
typedef CL_API_ENTRY void * CL_API_CALL
(*PFN_clSVMAlloc)(cl_context       /* context */,
           cl_svm_mem_flags /* flags */,
           size_t           /* size */,
           cl_uint          /* alignment */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY void CL_API_CALL
(*PFN_clSVMFree)(cl_context        /* context */,
          void *            /* svm_pointer */) CL_API_SUFFIX__VERSION_2_0;

/* Sampler APIs */
typedef CL_API_ENTRY cl_sampler CL_API_CALL
(*PFN_clCreateSamplerWithProperties)(cl_context                     /* context */,
                              const cl_sampler_properties *  /* normalized_coords */,
                              cl_int *                       /* errcode_ret */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clRetainSampler)(cl_sampler /* sampler */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clReleaseSampler)(cl_sampler /* sampler */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetSamplerInfo)(cl_sampler         /* sampler */,
                 cl_sampler_info    /* param_name */,
                 size_t             /* param_value_size */,
                 void *             /* param_value */,
                 size_t *           /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

/* Program Object APIs  */
typedef CL_API_ENTRY cl_program CL_API_CALL
(*PFN_clCreateProgramWithSource)(cl_context        /* context */,
                          cl_uint           /* count */,
                          const char **     /* strings */,
                          const size_t *    /* lengths */,
                          cl_int *          /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_program CL_API_CALL
(*PFN_clCreateProgramWithBinary)(cl_context                     /* context */,
                          cl_uint                        /* num_devices */,
                          const cl_device_id *           /* device_list */,
                          const size_t *                 /* lengths */,
                          const unsigned char **         /* binaries */,
                          cl_int *                       /* binary_status */,
                          cl_int *                       /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_program CL_API_CALL
(*PFN_clCreateProgramWithBuiltInKernels)(cl_context            /* context */,
                                  cl_uint               /* num_devices */,
                                  const cl_device_id *  /* device_list */,
                                  const char *          /* kernel_names */,
                                  cl_int *              /* errcode_ret */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clRetainProgram)(cl_program /* program */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clReleaseProgram)(cl_program /* program */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clBuildProgram)(cl_program           /* program */,
               cl_uint              /* num_devices */,
               const cl_device_id * /* device_list */,
               const char *         /* options */,
               void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
               void *               /* user_data */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clCompileProgram)(cl_program           /* program */,
                 cl_uint              /* num_devices */,
                 const cl_device_id * /* device_list */,
                 const char *         /* options */,
                 cl_uint              /* num_input_headers */,
                 const cl_program *   /* input_headers */,
                 const char **        /* header_include_names */,
                 void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
                 void *               /* user_data */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_program CL_API_CALL
(*PFN_clLinkProgram)(cl_context           /* context */,
              cl_uint              /* num_devices */,
              const cl_device_id * /* device_list */,
              const char *         /* options */,
              cl_uint              /* num_input_programs */,
              const cl_program *   /* input_programs */,
              void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
              void *               /* user_data */,
              cl_int *             /* errcode_ret */ ) CL_API_SUFFIX__VERSION_1_2;


typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clUnloadPlatformCompiler)(cl_platform_id /* platform */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetProgramInfo)(cl_program         /* program */,
                 cl_program_info    /* param_name */,
                 size_t             /* param_value_size */,
                 void *             /* param_value */,
                 size_t *           /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetProgramBuildInfo)(cl_program            /* program */,
                      cl_device_id          /* device */,
                      cl_program_build_info /* param_name */,
                      size_t                /* param_value_size */,
                      void *                /* param_value */,
                      size_t *              /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

/* Kernel Object APIs */
typedef CL_API_ENTRY cl_kernel CL_API_CALL
(*PFN_clCreateKernel)(cl_program      /* program */,
               const char *    /* kernel_name */,
               cl_int *        /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clCreateKernelsInProgram)(cl_program     /* program */,
                         cl_uint        /* num_kernels */,
                         cl_kernel *    /* kernels */,
                         cl_uint *      /* num_kernels_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clRetainKernel)(cl_kernel    /* kernel */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clReleaseKernel)(cl_kernel   /* kernel */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clSetKernelArg)(cl_kernel    /* kernel */,
               cl_uint      /* arg_index */,
               size_t       /* arg_size */,
               const void * /* arg_value */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clSetKernelArgSVMPointer)(cl_kernel    /* kernel */,
                         cl_uint      /* arg_index */,
                         const void * /* arg_value */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clSetKernelExecInfo)(cl_kernel            /* kernel */,
                    cl_kernel_exec_info  /* param_name */,
                    size_t               /* param_value_size */,
                    const void *         /* param_value */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetKernelInfo)(cl_kernel       /* kernel */,
                cl_kernel_info  /* param_name */,
                size_t          /* param_value_size */,
                void *          /* param_value */,
                size_t *        /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetKernelArgInfo)(cl_kernel       /* kernel */,
                   cl_uint         /* arg_indx */,
                   cl_kernel_arg_info  /* param_name */,
                   size_t          /* param_value_size */,
                   void *          /* param_value */,
                   size_t *        /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetKernelWorkGroupInfo)(cl_kernel                  /* kernel */,
                         cl_device_id               /* device */,
                         cl_kernel_work_group_info  /* param_name */,
                         size_t                     /* param_value_size */,
                         void *                     /* param_value */,
                         size_t *                   /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

/* Event Object APIs */
typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clWaitForEvents)(cl_uint             /* num_events */,
                const cl_event *    /* event_list */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetEventInfo)(cl_event         /* event */,
               cl_event_info    /* param_name */,
               size_t           /* param_value_size */,
               void *           /* param_value */,
               size_t *         /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_event CL_API_CALL
(*PFN_clCreateUserEvent)(cl_context    /* context */,
                  cl_int *      /* errcode_ret */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clRetainEvent)(cl_event /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clReleaseEvent)(cl_event /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clSetUserEventStatus)(cl_event   /* event */,
                     cl_int     /* execution_status */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clSetEventCallback)( cl_event    /* event */,
                    cl_int      /* command_exec_callback_type */,
                    void (CL_CALLBACK * /* pfn_notify */)(cl_event, cl_int, void *),
                    void *      /* user_data */) CL_API_SUFFIX__VERSION_1_1;

/* Profiling APIs */
typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clGetEventProfilingInfo)(cl_event            /* event */,
                        cl_profiling_info   /* param_name */,
                        size_t              /* param_value_size */,
                        void *              /* param_value */,
                        size_t *            /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

/* Flush and Finish APIs */
typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clFlush)(cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clFinish)(cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0;

/* Enqueued Commands APIs */
typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueReadBuffer)(cl_command_queue    /* command_queue */,
                    cl_mem              /* buffer */,
                    cl_bool             /* blocking_read */,
                    size_t              /* offset */,
                    size_t              /* size */,
                    void *              /* ptr */,
                    cl_uint             /* num_events_in_wait_list */,
                    const cl_event *    /* event_wait_list */,
                    cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueReadBufferRect)(cl_command_queue    /* command_queue */,
                        cl_mem              /* buffer */,
                        cl_bool             /* blocking_read */,
                        const size_t *      /* buffer_offset */,
                        const size_t *      /* host_offset */,
                        const size_t *      /* region */,
                        size_t              /* buffer_row_pitch */,
                        size_t              /* buffer_slice_pitch */,
                        size_t              /* host_row_pitch */,
                        size_t              /* host_slice_pitch */,
                        void *              /* ptr */,
                        cl_uint             /* num_events_in_wait_list */,
                        const cl_event *    /* event_wait_list */,
                        cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueWriteBuffer)(cl_command_queue   /* command_queue */,
                     cl_mem             /* buffer */,
                     cl_bool            /* blocking_write */,
                     size_t             /* offset */,
                     size_t             /* size */,
                     const void *       /* ptr */,
                     cl_uint            /* num_events_in_wait_list */,
                     const cl_event *   /* event_wait_list */,
                     cl_event *         /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueWriteBufferRect)(cl_command_queue    /* command_queue */,
                         cl_mem              /* buffer */,
                         cl_bool             /* blocking_write */,
                         const size_t *      /* buffer_offset */,
                         const size_t *      /* host_offset */,
                         const size_t *      /* region */,
                         size_t              /* buffer_row_pitch */,
                         size_t              /* buffer_slice_pitch */,
                         size_t              /* host_row_pitch */,
                         size_t              /* host_slice_pitch */,
                         const void *        /* ptr */,
                         cl_uint             /* num_events_in_wait_list */,
                         const cl_event *    /* event_wait_list */,
                         cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueFillBuffer)(cl_command_queue   /* command_queue */,
                    cl_mem             /* buffer */,
                    const void *       /* pattern */,
                    size_t             /* pattern_size */,
                    size_t             /* offset */,
                    size_t             /* size */,
                    cl_uint            /* num_events_in_wait_list */,
                    const cl_event *   /* event_wait_list */,
                    cl_event *         /* event */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueCopyBuffer)(cl_command_queue    /* command_queue */,
                    cl_mem              /* src_buffer */,
                    cl_mem              /* dst_buffer */,
                    size_t              /* src_offset */,
                    size_t              /* dst_offset */,
                    size_t              /* size */,
                    cl_uint             /* num_events_in_wait_list */,
                    const cl_event *    /* event_wait_list */,
                    cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueCopyBufferRect)(cl_command_queue    /* command_queue */,
                        cl_mem              /* src_buffer */,
                        cl_mem              /* dst_buffer */,
                        const size_t *      /* src_origin */,
                        const size_t *      /* dst_origin */,
                        const size_t *      /* region */,
                        size_t              /* src_row_pitch */,
                        size_t              /* src_slice_pitch */,
                        size_t              /* dst_row_pitch */,
                        size_t              /* dst_slice_pitch */,
                        cl_uint             /* num_events_in_wait_list */,
                        const cl_event *    /* event_wait_list */,
                        cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueReadImage)(cl_command_queue     /* command_queue */,
                   cl_mem               /* image */,
                   cl_bool              /* blocking_read */,
                   const size_t *       /* origin[3] */,
                   const size_t *       /* region[3] */,
                   size_t               /* row_pitch */,
                   size_t               /* slice_pitch */,
                   void *               /* ptr */,
                   cl_uint              /* num_events_in_wait_list */,
                   const cl_event *     /* event_wait_list */,
                   cl_event *           /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueWriteImage)(cl_command_queue    /* command_queue */,
                    cl_mem              /* image */,
                    cl_bool             /* blocking_write */,
                    const size_t *      /* origin[3] */,
                    const size_t *      /* region[3] */,
                    size_t              /* input_row_pitch */,
                    size_t              /* input_slice_pitch */,
                    const void *        /* ptr */,
                    cl_uint             /* num_events_in_wait_list */,
                    const cl_event *    /* event_wait_list */,
                    cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueFillImage)(cl_command_queue   /* command_queue */,
                   cl_mem             /* image */,
                   const void *       /* fill_color */,
                   const size_t *     /* origin[3] */,
                   const size_t *     /* region[3] */,
                   cl_uint            /* num_events_in_wait_list */,
                   const cl_event *   /* event_wait_list */,
                   cl_event *         /* event */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueCopyImage)(cl_command_queue     /* command_queue */,
                   cl_mem               /* src_image */,
                   cl_mem               /* dst_image */,
                   const size_t *       /* src_origin[3] */,
                   const size_t *       /* dst_origin[3] */,
                   const size_t *       /* region[3] */,
                   cl_uint              /* num_events_in_wait_list */,
                   const cl_event *     /* event_wait_list */,
                   cl_event *           /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueCopyImageToBuffer)(cl_command_queue /* command_queue */,
                           cl_mem           /* src_image */,
                           cl_mem           /* dst_buffer */,
                           const size_t *   /* src_origin[3] */,
                           const size_t *   /* region[3] */,
                           size_t           /* dst_offset */,
                           cl_uint          /* num_events_in_wait_list */,
                           const cl_event * /* event_wait_list */,
                           cl_event *       /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueCopyBufferToImage)(cl_command_queue /* command_queue */,
                           cl_mem           /* src_buffer */,
                           cl_mem           /* dst_image */,
                           size_t           /* src_offset */,
                           const size_t *   /* dst_origin[3] */,
                           const size_t *   /* region[3] */,
                           cl_uint          /* num_events_in_wait_list */,
                           const cl_event * /* event_wait_list */,
                           cl_event *       /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY void * CL_API_CALL
(*PFN_clEnqueueMapBuffer)(cl_command_queue /* command_queue */,
                   cl_mem           /* buffer */,
                   cl_bool          /* blocking_map */,
                   cl_map_flags     /* map_flags */,
                   size_t           /* offset */,
                   size_t           /* size */,
                   cl_uint          /* num_events_in_wait_list */,
                   const cl_event * /* event_wait_list */,
                   cl_event *       /* event */,
                   cl_int *         /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY void * CL_API_CALL
(*PFN_clEnqueueMapImage)(cl_command_queue  /* command_queue */,
                  cl_mem            /* image */,
                  cl_bool           /* blocking_map */,
                  cl_map_flags      /* map_flags */,
                  const size_t *    /* origin[3] */,
                  const size_t *    /* region[3] */,
                  size_t *          /* image_row_pitch */,
                  size_t *          /* image_slice_pitch */,
                  cl_uint           /* num_events_in_wait_list */,
                  const cl_event *  /* event_wait_list */,
                  cl_event *        /* event */,
                  cl_int *          /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueUnmapMemObject)(cl_command_queue /* command_queue */,
                        cl_mem           /* memobj */,
                        void *           /* mapped_ptr */,
                        cl_uint          /* num_events_in_wait_list */,
                        const cl_event *  /* event_wait_list */,
                        cl_event *        /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueMigrateMemObjects)(cl_command_queue       /* command_queue */,
                           cl_uint                /* num_mem_objects */,
                           const cl_mem *         /* mem_objects */,
                           cl_mem_migration_flags /* flags */,
                           cl_uint                /* num_events_in_wait_list */,
                           const cl_event *       /* event_wait_list */,
                           cl_event *             /* event */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueNDRangeKernel)(cl_command_queue /* command_queue */,
                       cl_kernel        /* kernel */,
                       cl_uint          /* work_dim */,
                       const size_t *   /* global_work_offset */,
                       const size_t *   /* global_work_size */,
                       const size_t *   /* local_work_size */,
                       cl_uint          /* num_events_in_wait_list */,
                       const cl_event * /* event_wait_list */,
                       cl_event *       /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueNativeKernel)(cl_command_queue  /* command_queue */,
                      void (CL_CALLBACK * /*user_func*/)(void *),
                      void *            /* args */,
                      size_t            /* cb_args */,
                      cl_uint           /* num_mem_objects */,
                      const cl_mem *    /* mem_list */,
                      const void **     /* args_mem_loc */,
                      cl_uint           /* num_events_in_wait_list */,
                      const cl_event *  /* event_wait_list */,
                      cl_event *        /* event */) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueMarkerWithWaitList)(cl_command_queue  /* command_queue */,
                            cl_uint           /* num_events_in_wait_list */,
                            const cl_event *  /* event_wait_list */,
                            cl_event *        /* event */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueBarrierWithWaitList)(cl_command_queue  /* command_queue */,
                             cl_uint           /* num_events_in_wait_list */,
                             const cl_event *  /* event_wait_list */,
                             cl_event *        /* event */) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueSVMFree)(cl_command_queue  /* command_queue */,
                 cl_uint           /* num_svm_pointers */,
                 void *[]          /* svm_pointers[] */,
                 void (CL_CALLBACK * /*pfn_free_func*/)(cl_command_queue /* queue */,
                                                        cl_uint          /* num_svm_pointers */,
                                                        void *[]         /* svm_pointers[] */,
                                                        void *           /* user_data */),
                 void *            /* user_data */,
                 cl_uint           /* num_events_in_wait_list */,
                 const cl_event *  /* event_wait_list */,
                 cl_event *        /* event */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueSVMMemcpy)(cl_command_queue  /* command_queue */,
                   cl_bool           /* blocking_copy */,
                   void *            /* dst_ptr */,
                   const void *      /* src_ptr */,
                   size_t            /* size */,
                   cl_uint           /* num_events_in_wait_list */,
                   const cl_event *  /* event_wait_list */,
                   cl_event *        /* event */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueSVMMemFill)(cl_command_queue  /* command_queue */,
                    void *            /* svm_ptr */,
                    const void *      /* pattern */,
                    size_t            /* pattern_size */,
                    size_t            /* size */,
                    cl_uint           /* num_events_in_wait_list */,
                    const cl_event *  /* event_wait_list */,
                    cl_event *        /* event */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueSVMMap)(cl_command_queue  /* command_queue */,
                cl_bool           /* blocking_map */,
                cl_map_flags      /* flags */,
                void *            /* svm_ptr */,
                size_t            /* size */,
                cl_uint           /* num_events_in_wait_list */,
                const cl_event *  /* event_wait_list */,
                cl_event *        /* event */) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int CL_API_CALL
(*PFN_clEnqueueSVMUnmap)(cl_command_queue  /* command_queue */,
                  void *            /* svm_ptr */,
                  cl_uint           /* num_events_in_wait_list */,
                  const cl_event *  /* event_wait_list */,
                  cl_event *        /* event */) CL_API_SUFFIX__VERSION_2_0;


/* Extension function access
 *
 * Returns the extension function address for the given function name,
 * or NULL if a valid function can not be found.  The client must
 * check to make sure the address is not NULL, before using or
 * calling the returned function address.
 */
typedef CL_API_ENTRY void * CL_API_CALL
(*PFN_clGetExtensionFunctionAddressForPlatform)(cl_platform_id /* platform */,
                                         const char *   /* func_name */) CL_API_SUFFIX__VERSION_1_2;

extern PFN_clGetPlatformIDs shim_clGetPlatformIDs;
extern PFN_clGetPlatformInfo shim_clGetPlatformInfo;
extern PFN_clGetDeviceIDs shim_clGetDeviceIDs;
extern PFN_clGetDeviceInfo shim_clGetDeviceInfo;
extern PFN_clCreateSubDevices shim_clCreateSubDevices;
extern PFN_clRetainDevice shim_clRetainDevice;
extern PFN_clReleaseDevice shim_clReleaseDevice;
extern PFN_clCreateContext shim_clCreateContext;
extern PFN_clCreateContextFromType shim_clCreateContextFromType;
extern PFN_clRetainContext shim_clRetainContext;
extern PFN_clReleaseContext shim_clReleaseContext;
extern PFN_clGetContextInfo shim_clGetContextInfo;
extern PFN_clCreateCommandQueueWithProperties shim_clCreateCommandQueueWithProperties;
extern PFN_clRetainCommandQueue shim_clRetainCommandQueue;
extern PFN_clReleaseCommandQueue shim_clReleaseCommandQueue;
extern PFN_clGetCommandQueueInfo shim_clGetCommandQueueInfo;
extern PFN_clCreateBuffer shim_clCreateBuffer;
extern PFN_clCreateSubBuffer shim_clCreateSubBuffer;
extern PFN_clCreateImage shim_clCreateImage;
extern PFN_clCreatePipe shim_clCreatePipe;
extern PFN_clRetainMemObject shim_clRetainMemObject;
extern PFN_clReleaseMemObject shim_clReleaseMemObject;
extern PFN_clGetSupportedImageFormats shim_clGetSupportedImageFormats;
extern PFN_clGetMemObjectInfo shim_clGetMemObjectInfo;
extern PFN_clGetImageInfo shim_clGetImageInfo;
extern PFN_clGetPipeInfo shim_clGetPipeInfo;
extern PFN_clSetMemObjectDestructorCallback shim_clSetMemObjectDestructorCallback;
extern PFN_clSVMAlloc shim_clSVMAlloc;
extern PFN_clSVMFree shim_clSVMFree;
extern PFN_clCreateSamplerWithProperties shim_clCreateSamplerWithProperties;
extern PFN_clRetainSampler shim_clRetainSampler;
extern PFN_clReleaseSampler shim_clReleaseSampler;
extern PFN_clGetSamplerInfo shim_clGetSamplerInfo;
extern PFN_clCreateProgramWithSource shim_clCreateProgramWithSource;
extern PFN_clCreateProgramWithBinary shim_clCreateProgramWithBinary;
extern PFN_clCreateProgramWithBuiltInKernels shim_clCreateProgramWithBuiltInKernels;
extern PFN_clRetainProgram shim_clRetainProgram;
extern PFN_clReleaseProgram shim_clReleaseProgram;
extern PFN_clBuildProgram shim_clBuildProgram;
extern PFN_clCompileProgram shim_clCompileProgram;
extern PFN_clLinkProgram shim_clLinkProgram;
extern PFN_clUnloadPlatformCompiler shim_clUnloadPlatformCompiler;
extern PFN_clGetProgramInfo shim_clGetProgramInfo;
extern PFN_clGetProgramBuildInfo shim_clGetProgramBuildInfo;
extern PFN_clCreateKernel shim_clCreateKernel;
extern PFN_clCreateKernelsInProgram shim_clCreateKernelsInProgram;
extern PFN_clRetainKernel shim_clRetainKernel;
extern PFN_clReleaseKernel shim_clReleaseKernel;
extern PFN_clSetKernelArg shim_clSetKernelArg;
extern PFN_clSetKernelArgSVMPointer shim_clSetKernelArgSVMPointer;
extern PFN_clSetKernelExecInfo shim_clSetKernelExecInfo;
extern PFN_clGetKernelInfo shim_clGetKernelInfo;
extern PFN_clGetKernelArgInfo shim_clGetKernelArgInfo;
extern PFN_clGetKernelWorkGroupInfo shim_clGetKernelWorkGroupInfo;
extern PFN_clWaitForEvents shim_clWaitForEvents;
extern PFN_clGetEventInfo shim_clGetEventInfo;
extern PFN_clCreateUserEvent shim_clCreateUserEvent;
extern PFN_clRetainEvent shim_clRetainEvent;
extern PFN_clReleaseEvent shim_clReleaseEvent;
extern PFN_clSetUserEventStatus shim_clSetUserEventStatus;
extern PFN_clSetEventCallback shim_clSetEventCallback;
extern PFN_clGetEventProfilingInfo shim_clGetEventProfilingInfo;
extern PFN_clFlush shim_clFlush;
extern PFN_clFinish shim_clFinish;
extern PFN_clEnqueueReadBuffer shim_clEnqueueReadBuffer;
extern PFN_clEnqueueReadBufferRect shim_clEnqueueReadBufferRect;
extern PFN_clEnqueueWriteBuffer shim_clEnqueueWriteBuffer;
extern PFN_clEnqueueWriteBufferRect shim_clEnqueueWriteBufferRect;
extern PFN_clEnqueueFillBuffer shim_clEnqueueFillBuffer;
extern PFN_clEnqueueCopyBuffer shim_clEnqueueCopyBuffer;
extern PFN_clEnqueueCopyBufferRect shim_clEnqueueCopyBufferRect;
extern PFN_clEnqueueReadImage shim_clEnqueueReadImage;
extern PFN_clEnqueueWriteImage shim_clEnqueueWriteImage;
extern PFN_clEnqueueFillImage shim_clEnqueueFillImage;
extern PFN_clEnqueueCopyImage shim_clEnqueueCopyImage;
extern PFN_clEnqueueCopyImageToBuffer shim_clEnqueueCopyImageToBuffer;
extern PFN_clEnqueueCopyBufferToImage shim_clEnqueueCopyBufferToImage;
extern PFN_clEnqueueMapBuffer shim_clEnqueueMapBuffer;
extern PFN_clEnqueueMapImage shim_clEnqueueMapImage;
extern PFN_clEnqueueUnmapMemObject shim_clEnqueueUnmapMemObject;
extern PFN_clEnqueueMigrateMemObjects shim_clEnqueueMigrateMemObjects;
extern PFN_clEnqueueNDRangeKernel shim_clEnqueueNDRangeKernel;
extern PFN_clEnqueueNativeKernel shim_clEnqueueNativeKernel;
extern PFN_clEnqueueMarkerWithWaitList shim_clEnqueueMarkerWithWaitList;
extern PFN_clEnqueueBarrierWithWaitList shim_clEnqueueBarrierWithWaitList;
extern PFN_clEnqueueSVMFree shim_clEnqueueSVMFree;
extern PFN_clEnqueueSVMMemcpy shim_clEnqueueSVMMemcpy;
extern PFN_clEnqueueSVMMemFill shim_clEnqueueSVMMemFill;
extern PFN_clEnqueueSVMMap shim_clEnqueueSVMMap;
extern PFN_clEnqueueSVMUnmap shim_clEnqueueSVMUnmap;
extern PFN_clGetExtensionFunctionAddressForPlatform shim_clGetExtensionFunctionAddressForPlatform;

#define clGetPlatformIDs shim_clGetPlatformIDs
#define clGetPlatformInfo shim_clGetPlatformInfo
#define clGetDeviceIDs shim_clGetDeviceIDs
#define clGetDeviceInfo shim_clGetDeviceInfo
#define clCreateSubDevices shim_clCreateSubDevices
#define clRetainDevice shim_clRetainDevice
#define clReleaseDevice shim_clReleaseDevice
#define clCreateContext shim_clCreateContext
#define clCreateContextFromType shim_clCreateContextFromType
#define clRetainContext shim_clRetainContext
#define clReleaseContext shim_clReleaseContext
#define clGetContextInfo shim_clGetContextInfo
#define clCreateCommandQueueWithProperties shim_clCreateCommandQueueWithProperties
#define clRetainCommandQueue shim_clRetainCommandQueue
#define clReleaseCommandQueue shim_clReleaseCommandQueue
#define clGetCommandQueueInfo shim_clGetCommandQueueInfo
#define clCreateBuffer shim_clCreateBuffer
#define clCreateSubBuffer shim_clCreateSubBuffer
#define clCreateImage shim_clCreateImage
#define clCreatePipe shim_clCreatePipe
#define clRetainMemObject shim_clRetainMemObject
#define clReleaseMemObject shim_clReleaseMemObject
#define clGetSupportedImageFormats shim_clGetSupportedImageFormats
#define clGetMemObjectInfo shim_clGetMemObjectInfo
#define clGetImageInfo shim_clGetImageInfo
#define clGetPipeInfo shim_clGetPipeInfo
#define clSetMemObjectDestructorCallback shim_clSetMemObjectDestructorCallback
#define clSVMAlloc shim_clSVMAlloc
#define clSVMFree shim_clSVMFree
#define clCreateSamplerWithProperties shim_clCreateSamplerWithProperties
#define clRetainSampler shim_clRetainSampler
#define clReleaseSampler shim_clReleaseSampler
#define clGetSamplerInfo shim_clGetSamplerInfo
#define clCreateProgramWithSource shim_clCreateProgramWithSource
#define clCreateProgramWithBinary shim_clCreateProgramWithBinary
#define clCreateProgramWithBuiltInKernels shim_clCreateProgramWithBuiltInKernels
#define clRetainProgram shim_clRetainProgram
#define clReleaseProgram shim_clReleaseProgram
#define clBuildProgram shim_clBuildProgram
#define clCompileProgram shim_clCompileProgram
#define clLinkProgram shim_clLinkProgram
#define clUnloadPlatformCompiler shim_clUnloadPlatformCompiler
#define clGetProgramInfo shim_clGetProgramInfo
#define clGetProgramBuildInfo shim_clGetProgramBuildInfo
#define clCreateKernel shim_clCreateKernel
#define clCreateKernelsInProgram shim_clCreateKernelsInProgram
#define clRetainKernel shim_clRetainKernel
#define clReleaseKernel shim_clReleaseKernel
#define clSetKernelArg shim_clSetKernelArg
#define clSetKernelArgSVMPointer shim_clSetKernelArgSVMPointer
#define clSetKernelExecInfo shim_clSetKernelExecInfo
#define clGetKernelInfo shim_clGetKernelInfo
#define clGetKernelArgInfo shim_clGetKernelArgInfo
#define clGetKernelWorkGroupInfo shim_clGetKernelWorkGroupInfo
#define clWaitForEvents shim_clWaitForEvents
#define clGetEventInfo shim_clGetEventInfo
#define clCreateUserEvent shim_clCreateUserEvent
#define clRetainEvent shim_clRetainEvent
#define clReleaseEvent shim_clReleaseEvent
#define clSetUserEventStatus shim_clSetUserEventStatus
#define clSetEventCallback shim_clSetEventCallback
#define clGetEventProfilingInfo shim_clGetEventProfilingInfo
#define clFlush shim_clFlush
#define clFinish shim_clFinish
#define clEnqueueReadBuffer shim_clEnqueueReadBuffer
#define clEnqueueReadBufferRect shim_clEnqueueReadBufferRect
#define clEnqueueWriteBuffer shim_clEnqueueWriteBuffer
#define clEnqueueWriteBufferRect shim_clEnqueueWriteBufferRect
#define clEnqueueFillBuffer shim_clEnqueueFillBuffer
#define clEnqueueCopyBuffer shim_clEnqueueCopyBuffer
#define clEnqueueCopyBufferRect shim_clEnqueueCopyBufferRect
#define clEnqueueReadImage shim_clEnqueueReadImage
#define clEnqueueWriteImage shim_clEnqueueWriteImage
#define clEnqueueFillImage shim_clEnqueueFillImage
#define clEnqueueCopyImage shim_clEnqueueCopyImage
#define clEnqueueCopyImageToBuffer shim_clEnqueueCopyImageToBuffer
#define clEnqueueCopyBufferToImage shim_clEnqueueCopyBufferToImage
#define clEnqueueMapBuffer shim_clEnqueueMapBuffer
#define clEnqueueMapImage shim_clEnqueueMapImage
#define clEnqueueUnmapMemObject shim_clEnqueueUnmapMemObject
#define clEnqueueMigrateMemObjects shim_clEnqueueMigrateMemObjects
#define clEnqueueNDRangeKernel shim_clEnqueueNDRangeKernel
#define clEnqueueNativeKernel shim_clEnqueueNativeKernel
#define clEnqueueMarkerWithWaitList shim_clEnqueueMarkerWithWaitList
#define clEnqueueBarrierWithWaitList shim_clEnqueueBarrierWithWaitList
#define clEnqueueSVMFree shim_clEnqueueSVMFree
#define clEnqueueSVMMemcpy shim_clEnqueueSVMMemcpy
#define clEnqueueSVMMemFill shim_clEnqueueSVMMemFill
#define clEnqueueSVMMap shim_clEnqueueSVMMap
#define clEnqueueSVMUnmap shim_clEnqueueSVMUnmap
#define clGetExtensionFunctionAddressForPlatform shim_clGetExtensionFunctionAddressForPlatform
