#include "ljmgpu_gpu.h"
#include "ljmgpu_log.h"
#include "ljmgpu_util.h"

/*************************************************************************
@brief:打包vbo缓存数据
@details:添加包头、数据计数、包尾、校验，与command数据包不同的是，
vbo数据包仅在包尾进行校验。
直接发送纹理图像raw数据到vbo对应地址段，不对纹理数据进行打包处理
@param void* data 缓存数据
@param int data_cnt 数据量（DWORD数）
@param unsigned head 包头标记（0xffffffff，0xfff0000）
@param unsigned tail 包尾标记（0xf0f0f0f0）
/*************************************************************************/
void* ljmgpu_vbo_add_head_tail_checksum(void* data, int data_cnt, unsigned head, unsigned tail)
{    
    if (!data) {
        LOGE("In _vbo_add_head_tail_checksum: data is NULL!\n");
        return NULL;
    }
    int dword_cnt = data_cnt + 4; // head, cnt, tail, checksum
    unsigned *new_data = (unsigned *)malloc(dword_cnt * sizeof(unsigned));
    if (!new_data) {
        LOGE("In _vbo_add_head_tail_checksum: malloc new_data failed!\n");
        return NULL;
    }
    unsigned *ptr = new_data;
    *(ptr++) = head;
    *(ptr++) = data_cnt; // cnt
    memcpy(ptr, data, data_cnt * sizeof(unsigned));
    ptr += data_cnt; // data
    *(ptr++) = tail;
    *(ptr++) = checksum_xor(new_data, dword_cnt - 1);

    return new_data;
}

/// @brief 打包command缓存数据.
/// @details 添加包头、数据计数、包尾、校验，与vbo数据不同的是，command打包数据每32个DWORD进行一次校验。
/// @param buf 缓存块对象
/// @param head 包头标记（0xffffffff、0xfff0000）
/// @param tail 包尾标记（0xf0f0f0f0）
/// @param packet 打包后的数据
void ljmgpu_cmd_add_head_tail_checksums(struct util_dynarray * buf, unsigned head, unsigned tail, struct util_dynarray * packet)
{
	if (!buf->data) {
		LOGE("In _cmd_add_head_tail_checksums: buf->data is NULL!\n");
		return;
	}

    // checksum count, +2 means with header, cnt
	int checksums = (2 + (buf->size / sizeof(unsigned))) / 32 + 1;

    // New data format:
	// (head, cnt, data(with first_checksum and middle_checksum), tail, last_checksum
	//   1     1        buf->cnt + checksums-1                     1       1
	int dword_cnt = (2 + (buf->size / sizeof(unsigned))) + 1 + checksums; // total dword count with all checksums
	//int last_checksum_dword_cnt = dword_cnt - 32 * (checksums - 1);
	int last_checksum_dword_cnt = (buf->size / sizeof(unsigned)) - 30 - 32 * (checksums - 2);

    if (dword_cnt <= 0)
		return;

    // Create new_data as storage (with data and its checksums).
    unsigned *new_data = (unsigned *)malloc(dword_cnt * sizeof(unsigned));
    if (!new_data) {
		LOGE("In _cmd_add_head_tail_checksums: malloc new_data failed!\n");
		return;
    }
    unsigned *ptr = new_data;

	// "tmp" stores each 32 dwords (for one checksum at 33rd dword).
    unsigned *tmp = (unsigned *)malloc(32 * sizeof(unsigned));
    if (!tmp) {
		LOGE("In _cmd_add_head_tail_checksums: malloc tmp_data failed!\n");
		return;
    }
    /*memset(tmp, 0, 32 * sizeof(unsigned));*/

    int tmp_cnt = 0;
    for (int i = 0; i < checksums; i++) {
		memset(tmp, 0, 32 * sizeof(unsigned));
		if (i == 0) { // the first checksum
			*tmp = head;
			*(tmp + 1) = dword_cnt;
			if (checksums == 1) { // if only one checksum, dwords may less than 32
				memcpy(tmp + 2, (unsigned *)buf->data, buf->size);
				tmp_cnt = 2 + (buf->size / sizeof(unsigned));
				*(tmp + tmp_cnt) = tail;
				tmp_cnt = tmp_cnt + 1;
			} else {
				memcpy(tmp + 2, (unsigned *)buf->data, 30 * sizeof(unsigned));
				tmp_cnt = 32;
			}
		} else if (i == checksums - 1) { // the last checksum
            memcpy(tmp, (unsigned *)buf->data + 32 * (i - 1) + 30 * i, last_checksum_dword_cnt * sizeof(unsigned));
			tmp_cnt = last_checksum_dword_cnt;
	        //*(tmp + tmp_cnt - 1) = tail;
	        *(tmp + tmp_cnt) = tail;
			tmp_cnt = tmp_cnt + 1;
		} else { // middle checksums
			memcpy(tmp, (unsigned *)buf->data + 32 * (i - 1) + 30 * i, 32 * sizeof(unsigned));
			tmp_cnt = 32;
        }

        // Calculate a checksum.
	    unsigned tmp_checksum = checksum_xor(tmp, 32);

        if (tmp_cnt > dword_cnt) {
			LOGE("In _cmd_add_head_tail_checksums: tmp_cnt > dword_cnt!\n");
            return;
        }

        // Save data to new_data.
	    memcpy(ptr, tmp, tmp_cnt*sizeof(unsigned));
        ptr += tmp_cnt;

        // Save checksum to new_data.
	    *(ptr++) = tmp_checksum;
    }

    free(tmp);
    tmp = NULL;
    packet->data = new_data;
    packet->size = dword_cnt * sizeof(unsigned);
}
