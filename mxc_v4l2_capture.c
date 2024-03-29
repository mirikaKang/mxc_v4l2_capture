/*
 * Copyright 2004-2016 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file mxc_v4l2_capture.c
 *
 * @brief Mxc Video For Linux 2 driver test application
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <errno.h>

/* Verification Test Environment Include Files */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/videodev2.h>
// #include <linux/mxc_v4l2.h>
#include "mxc_v4l2.h"
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>

#define TEST_BUFFER_NUM  3
#define MAX_PLANE_COUNT  3
#define MAX_BUFFER_COUNT 4      

        struct testbuffer
        {
                unsigned char *start;
                size_t offset;
                unsigned int length;
        };

        struct plane_buffer
        {
            int plane_num;
            int fds[MAX_PLANE_COUNT];
            void *virt[MAX_PLANE_COUNT];
            unsigned long phys[MAX_PLANE_COUNT];
            int sizes[MAX_PLANE_COUNT];
        };


        struct buffer
        {
            void *start;
            size_t length;
        };
         


        // struct testbuffer buffers[TEST_BUFFER_NUM];
        int g_in_width = 176;
        int g_in_height = 144;
        int g_out_width = 176;
        int g_out_height = 144;
        int g_top = 0;
        int g_left = 0;
        int g_input = 0;
        int g_capture_count = 100;
        int g_rotate = 0;
        uint32_t g_cap_fmt = V4L2_PIX_FMT_YUV422P;
        int g_camera_framerate = 30;
        int g_extra_pixel = 0;
        int g_capture_mode = 0;
        int g_usb_camera = 0;
        char g_v4l_device[100] = "/dev/video0";
        struct plane_buffer g_plane_buffer[MAX_BUFFER_COUNT];
        struct buffer  g_buffer[MAX_BUFFER_COUNT];
        uint32_t g_buffers;


        static inline void print_name(char *const argv[])
        {
                printf("\n---- Running < %s > test ----\n\n", argv[0]);
        }

        static inline void print_result(char *const argv[])
        {
                printf("\n----  Test < %s > ended  ----\n\n", argv[0]);
        }

        static void print_pixelformat(char *prefix, int val)
        {
                printf("%s: %c%c%c%c\n", prefix ? prefix : "pixelformat",
                       val & 0xff,
                       (val >> 8) & 0xff,
                       (val >> 16) & 0xff,
                       (val >> 24) & 0xff);
        }

        int start_capturing(int fd_v4l)
        {
                unsigned int i;
                enum v4l2_buf_type type;
                printf("[%s:%d] v4l2 handle=%d\n", __func__, __LINE__, fd_v4l);

                for (g_buffers = 0; g_buffers < MAX_BUFFER_COUNT; ++g_buffers)
                {
                        struct v4l2_buffer buf;
                        struct v4l2_plane planes[MAX_PLANE_COUNT];
                        memset(&planes[0],0,(sizeof(struct v4l2_plane) * MAX_PLANE_COUNT ));
                        memset(&buf, 0, sizeof(buf));
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = g_buffers;
                        buf.length = MAX_PLANE_COUNT;
                        buf.m.planes = planes;
                        if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &buf) < 0)
                        {
                                printf("VIDIOC_QUERYBUF error\n");
                                return -1;
                        
                        }

                        for( int i=0;i<MAX_PLANE_COUNT;i++)
                        {
                                g_plane_buffer[g_buffers].sizes[i] = planes[i].length;
                                g_plane_buffer[g_buffers].virt[i] = mmap(NULL, 
                                                                        planes[i].length,
                                                                        PROT_READ | PROT_WRITE, 
                                                                        MAP_SHARED,
                                                                        fd_v4l, planes[i].m.mem_offset);
                                if( MAP_FAILED == g_plane_buffer[g_buffers].virt[i])
                                {
                                        printf("[%s:%d] MMAP Error \n");
                                        return -1;
                                }
                        }
                }

                // queue buffers
                for (int i = 0; i < g_buffers; i++)
                {
                        struct v4l2_buffer buf;
                        struct v4l2_plane planes[MAX_PLANE_COUNT];
                        memset(&buf, 0, sizeof(buf));
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;
                        buf.length = MAX_PLANE_COUNT;
                        memset(&planes[0],0,(sizeof(struct v4l2_plane)*MAX_PLANE_COUNT));
                        buf.m.planes = &planes[0];
                        if (ioctl(fd_v4l, VIDIOC_QBUF, &buf) < 0)
                        {
                                printf("VIDIOC_QBUF error\n");
                                return -1;
                        }
                }

                type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                if (ioctl(fd_v4l, VIDIOC_STREAMON, &type) < 0)
                {
                        printf("VIDIOC_STREAMON error\n");
                        return -1;
                }
                return 0;
        }

        int stop_capturing(int fd_v4l)
        {
                enum v4l2_buf_type type;

                type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                return ioctl(fd_v4l, VIDIOC_STREAMOFF, &type);
        }

        int v4l_capture_setup(void)
        {
                struct v4l2_format fmt;
                struct v4l2_control ctrl;
                struct v4l2_streamparm parm;
                struct v4l2_crop crop;
                struct v4l2_mxc_dest_crop of;
                // struct v4l2_dbg_chip_ident chip;
                struct v4l2_frmsizeenum fsize;
                struct v4l2_fmtdesc ffmt;
                int fd_v4l = 0;
                int ret;

                memset(&fmt, 0, sizeof(fmt));
                memset(&ctrl, 0, sizeof(ctrl));
                memset(&parm, 0, sizeof(parm));
                memset(&crop, 0, sizeof(crop));
                memset(&of, 0, sizeof(of));
                // memset(&chip, 0, sizeof(chip));
                memset(&fsize, 0, sizeof(fsize));
                memset(&ffmt, 0, sizeof(ffmt));

                if ((fd_v4l = open(g_v4l_device, O_RDWR, 0)) < 0)
                {
                        printf("Unable to open %s\n", g_v4l_device);
                        return 0;
                }

#if 0

	/* UVC driver does not support this ioctl */
	if (g_usb_camera != 1) {
		if (ioctl(fd_v4l, VIDIOC_DBG_G_CHIP_IDENT, &chip))
		{
			printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
			return -1;
		}
		printf("sensor chip is %s\n", chip.match.name);
	}
#endif

                sleep(1);
                printf("sensor supported frame size:\n");
                fsize.index = 0;
                fsize.pixel_format = g_cap_fmt;
                while (ioctl(fd_v4l, VIDIOC_ENUM_FRAMESIZES, &fsize) >= 0)
                {
                        printf(" %dx%d\n", fsize.discrete.width, fsize.discrete.height);
                        fsize.index++;
                }
                printf("sensor supported frame size index=%d:\n", fsize.index);

                sleep(1);
                ffmt.index = 0;
                while (ioctl(fd_v4l, VIDIOC_ENUM_FMT, &ffmt) >= 0)
                {
                        print_pixelformat("sensor frame format", ffmt.pixelformat);
                        ffmt.index++;
                }
                printf("sensor supported FMT index=%d:\n", ffmt.index);

                sleep(1);
                parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                parm.parm.capture.timeperframe.numerator = 1;
                parm.parm.capture.timeperframe.denominator = g_camera_framerate;
                parm.parm.capture.capturemode = g_capture_mode;

                printf("[%s:%d] VIDIOC_S_PARM \n", __func__, __LINE__);
                printf("[%s:%d] parm.type=0x%x\n", __func__, __LINE__, parm.type);
                printf("[%s:%d] parm.parm.capture.timeperframe.numerator=%d\n", __func__, __LINE__, parm.parm.capture.timeperframe.numerator);
                printf("[%s:%d] parm.parm.capture.timeperframe.denominator=%d\n", __func__, __LINE__, parm.parm.capture.timeperframe.denominator);
                printf("[%s:%d] parm.parm.capture.capturemode=0x%x\n", __func__, __LINE__, parm.parm.capture.capturemode);

                if ((ret = ioctl(fd_v4l, VIDIOC_S_PARM, &parm)) < 0)
                {
                        printf("VIDIOC_S_PARM failed ret=%d\n", ret);
                        return -1;
                }

#if 0
	if (ioctl(fd_v4l, VIDIOC_S_INPUT, &g_input) < 0)
	{
		printf("VIDIOC_S_INPUT failed\n");
		return -1;
	}
#endif

#if 0

                /* UVC driver does not implement CROP */
                if (g_usb_camera != 1)
                {
                        printf("[%s:%d] VIDIOC_G_CROP \n",__func__,__LINE__);
                        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        if (ioctl(fd_v4l, VIDIOC_G_CROP, &crop) < 0)
                        {
                                printf("VIDIOC_G_CROP failed\n");
                                return -1;
                        }

                        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        crop.c.width = g_in_width;
                        crop.c.height = g_in_height;
                        crop.c.top = g_top;
                        crop.c.left = g_left;
                        if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0)
                        {
                                printf("VIDIOC_S_CROP failed\n");
                                return -1;
                        }
                }

                of.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (g_extra_pixel)
                {
                        of.offset.u_offset = (2 * g_extra_pixel + g_out_width) * (g_out_height + g_extra_pixel) - g_extra_pixel + (g_extra_pixel / 2) * ((g_out_width / 2) + g_extra_pixel) + g_extra_pixel / 2;
                        of.offset.v_offset = of.offset.u_offset + (g_extra_pixel + g_out_width / 2) *
                                                                      ((g_out_height / 2) + g_extra_pixel);
                }
                else
                {
                        of.offset.u_offset = 0;
                        of.offset.v_offset = 0;
                }

                if (g_usb_camera != 1)
                {
                        if (ioctl(fd_v4l, VIDIOC_S_DEST_CROP, &of) < 0)
                        {
                                printf("set dest crop failed\n");
                                return 0;
                        }
                }
#endif

                fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                fmt.fmt.pix.pixelformat = g_cap_fmt;
                fmt.fmt.pix.width = g_out_width;
                fmt.fmt.pix.height = g_out_height;
                if (g_extra_pixel)
                {
                        fmt.fmt.pix.bytesperline = g_out_width + g_extra_pixel * 2;
                        fmt.fmt.pix.sizeimage = (g_out_width + g_extra_pixel * 2) * (g_out_height + g_extra_pixel * 2) * 3 / 2;
                }
                else
                {
                        fmt.fmt.pix.bytesperline = g_out_width;
                        fmt.fmt.pix.sizeimage = 0;
                }
                sleep(1);
                printf("[%s:%d] VIDIOC_S_FMT \n", __func__, __LINE__);
                printf("[%s:%d] fmt.type=0x%x\n", __func__, __LINE__, fmt.type);
                printf("[%s:%d] fmt.fmt.pix.pixelformat=0x%x\n", __func__, __LINE__, fmt.fmt.pix.pixelformat);
                printf("[%s:%d] fmt.fmt.pix.width=%d\n", __func__, __LINE__, fmt.fmt.pix.width);
                printf("[%s:%d] fmt.fmt.pix.height=%d\n", __func__, __LINE__, fmt.fmt.pix.height);
                if ((ret = ioctl(fd_v4l, VIDIOC_S_FMT, &fmt)) < 0)
                {
                        printf("[%s:%d] set format failed, ret=%d\n", __func__, __LINE__, ret);
                        return 0;
                }

#if 0         

                /*
                 * Set rotation
                 * It's mxc-specific definition for rotation.
                 */
                if (g_usb_camera != 1)
                {
                        ctrl.id = V4L2_CID_PRIVATE_BASE + 0;
                        ctrl.value = g_rotate;
                        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
                        {
                                printf("set ctrl failed\n");
                                return 0;
                        }
                }
#endif
                sleep(1);
                struct v4l2_requestbuffers req;
                memset(&req, 0, sizeof(req));
                req.count = MAX_BUFFER_COUNT;
                req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                req.memory = V4L2_MEMORY_MMAP;

                printf("[%s:%d] VIDIOC_REQBUFS \n", __func__, __LINE__);
                printf("[%s:%d] req.count=0x%x\n", __func__, __LINE__, req.count);
                printf("[%s:%d] req.type=0x%x\n", __func__, __LINE__, req.type);
                printf("[%s:%d] req.memory=0x%x\n", __func__, __LINE__, req.memory);
                if ((ret = ioctl(fd_v4l, VIDIOC_REQBUFS, &req)) < 0)
                {
                        printf("[%s:%d] v4l_capture_setup: VIDIOC_REQBUFS failed, ret=%d\n", __func__, __LINE__, ret);
                        return 0;
                }

                return fd_v4l;
        }

        int v4l_capture_test(int fd_v4l, const char *file)
        {
                struct v4l2_buffer buf;
                struct v4l2_plane planes[MAX_PLANE_COUNT];
#if TEST_OUTSYNC_ENQUE
                struct v4l2_buffer temp_buf;
#endif
                struct v4l2_format fmt;
                FILE *fd_y_file = 0;
                size_t wsize;
                int count = g_capture_count;
                printf("[%s:%d] write to %s\n", __func__, __LINE__, file);

                if ((fd_y_file = fopen(file, "wb")) == NULL)
                {
                        printf("Unable to create y frame recording file\n");
                        return -1;
                }
                printf("[%s:%d] File %s open succeed \n", __func__, __LINE__, file);

              
                memset(&fmt, 0, sizeof(fmt));
                fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                if (ioctl(fd_v4l, VIDIOC_G_FMT, &fmt) < 0)
                {
                        printf("get format failed\n");
                        return -1;
                }
                else
                {
                        printf("\t Width = %d", fmt.fmt.pix.width);
                        printf("\t Height = %d", fmt.fmt.pix.height);
                        printf("\t Image size = %d\n", fmt.fmt.pix.sizeimage);
                        print_pixelformat(0, fmt.fmt.pix.pixelformat);
                }

                printf("[%s:%d] Start capturing \n", __func__, __LINE__, file);
                if (start_capturing(fd_v4l) < 0)
                {
                        printf("start_capturing failed\n");
                        return -1;
                }

                while (count-- > 0)
                {
                        memset(&buf, 0, sizeof(buf));
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        if (ioctl(fd_v4l, VIDIOC_DQBUF, &buf) < 0)
                        {
                                printf("VIDIOC_DQBUF failed.\n");
                        }

                        // wsize = fwrite(buffers[buf.index].start, fmt.fmt.pix.sizeimage, 1, fd_y_file);
                        if (wsize < 1)
                        {
                                printf("No space left on device. Stopping after %d frames.\n",
                                       g_capture_count - (count + 1));
                                break;
                        }

#if TEST_OUTSYNC_ENQUE
                        /* Testing out of order enque */
                        if (count == 25)
                        {
                                temp_buf = buf;
                                printf("buf.index %d\n", buf.index);
                                continue;
                        }

                        if (count == 15)
                        {
                                if (ioctl(fd_v4l, VIDIOC_QBUF, &temp_buf) < 0)
                                {
                                        printf("VIDIOC_QBUF failed\n");
                                        break;
                                }
                        }
#endif
                        if (count >= TEST_BUFFER_NUM)
                        {
                                if (ioctl(fd_v4l, VIDIOC_QBUF, &buf) < 0)
                                {
                                        printf("VIDIOC_QBUF failed\n");
                                        break;
                                }
                        }
                        else
                                printf("buf.index %d\n", buf.index);
                }

                if (stop_capturing(fd_v4l) < 0)
                {
                        printf("stop_capturing failed\n");
                        return -1;
                }

                fclose(fd_y_file);

                close(fd_v4l);
                return 0;
        }

        int process_cmdline(int argc, char **argv)
        {
                int i;

                for (i = 1; i < argc; i++)
                {
                        if (strcmp(argv[i], "-iw") == 0)
                        {
                                g_in_width = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-ih") == 0)
                        {
                                g_in_height = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-ow") == 0)
                        {
                                g_out_width = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-oh") == 0)
                        {
                                g_out_height = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-t") == 0)
                        {
                                g_top = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-l") == 0)
                        {
                                g_left = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-i") == 0)
                        {
                                g_input = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-r") == 0)
                        {
                                g_rotate = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-c") == 0)
                        {
                                g_capture_count = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-fr") == 0)
                        {
                                g_camera_framerate = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-e") == 0)
                        {
                                g_extra_pixel = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-m") == 0)
                        {
                                g_capture_mode = atoi(argv[++i]);
                        }
                        else if (strcmp(argv[i], "-d") == 0)
                        {
                                strcpy(g_v4l_device, argv[++i]);
                        }
                        else if (strcmp(argv[i], "-f") == 0)
                        {
                                i++;
                                g_cap_fmt = v4l2_fourcc(argv[i][0], argv[i][1], argv[i][2], argv[i][3]);

                                printf("[%s:%d] g_vap_fmt 0x%x , YUV422P=0x%x\n", __func__, __LINE__, g_cap_fmt, V4L2_PIX_FMT_YUV422P);
                                if ((g_cap_fmt != V4L2_PIX_FMT_BGR24) &&
                                    (g_cap_fmt != V4L2_PIX_FMT_BGR32) &&
                                    (g_cap_fmt != V4L2_PIX_FMT_RGB565) &&
                                    (g_cap_fmt != V4L2_PIX_FMT_NV12) &&
                                    (g_cap_fmt != V4L2_PIX_FMT_YUV422P) &&
                                    (g_cap_fmt != V4L2_PIX_FMT_UYVY) &&
                                    (g_cap_fmt != V4L2_PIX_FMT_YUYV) &&
                                    (g_cap_fmt != V4L2_PIX_FMT_YVU420) &&
                                    (g_cap_fmt != V4L2_PIX_FMT_YUV420))
                                {
                                        printf("[%s:%d] -----------\n", __func__, __LINE__);
                                        return -1;
                                }
                        }
                        else if (strcmp(argv[i], "-uvc") == 0)
                        {
                                g_usb_camera = 1;
                        }
                        else if (strcmp(argv[i], "-help") == 0)
                        {
                                printf("MXC Video4Linux capture Device Test\n\n"
                                       "Syntax: mxc_v4l2_capture.out -iw <capture croped width>\n"
                                       " -ih <capture cropped height>\n"
                                       " -ow <capture output width>\n"
                                       " -oh <capture output height>\n"
                                       " -t <capture top>\n"
                                       " -l <capture left>\n"
                                       " -i <input mode, 0-use csi->prp_enc->mem, 1-use csi->mem>\n"
                                       " -r <rotation> -c <capture counter> \n"
                                       " -e <destination cropping: extra pixels> \n"
                                       " -m <capture mode, 0-low resolution, 1-high resolution> \n"
                                       " -d <camera select, /dev/video0, /dev/video1> \n"
                                       " -uvc (for USB camera test) \n"
                                       " -f <format> -fr <frame rate, 30fps by default> \n");
                                return -1;
                        }
                }

                printf("in_width = %d, in_height = %d\n", g_in_width, g_in_height);
                printf("out_width = %d, out_height = %d\n", g_out_width, g_out_height);
                printf("top = %d, left = %d\n", g_top, g_left);
                printf("fotmat=0x%x \n", g_cap_fmt);

                if ((g_in_width == 0) || (g_in_height == 0))
                {
                        return -1;
                }
                return 0;
        }

        int main(int argc, char **argv)
        {
                int fd_v4l;

                // print_name(argv);

                if (process_cmdline(argc, argv) < 0)
                {
                        printf("[%s:%d] cmd line get err\n", __func__, __LINE__);
                        return -1;
                }
                fd_v4l = v4l_capture_setup();
                if (fd_v4l)
                {
                        printf("vl4_capture_setup is done\n");
                        return v4l_capture_test(fd_v4l, argv[argc - 1]);
                }
        }
