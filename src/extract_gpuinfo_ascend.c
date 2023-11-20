/*
 * Copyright (C) 2023 klayer <klayer@163.com>
 *
 * This file is part of Nvtop and adapted from Ascend DCMI from Huawei Technologies Co., Ltd.
 *
 * Nvtop is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nvtop is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nvtop.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "ascend/dcmi_interface_api.h"
#include "list.h"
#include "nvtop/common.h"
#include "nvtop/extract_gpuinfo_common.h"

#define DCMI_SUCCESS 0
#define MAX_DEVICE_NUM 64
#define MAX_PROC_NUM 32
#define PROC_ALLOC_INC 16

/*
typedef int dcmiReturn_t;

dcmiReturn_t (*dcmi_init_func)(void);

dcmiReturn_t (*dcmi_get_card_num_list_func)(int *card_num, int *card_list, int list_length);

dcmiReturn_t (*dcmi_get_device_num_in_card_func)(int card_id, int *device_num);

dcmiReturn_t (*dcmi_get_device_logic_id_func)(int *device_logic_id, int card_id, int device_id);

dcmiReturn_t (*dcmi_create_vdevice_func)(int card_id, int device_id, struct dcmi_create_vdev_res_stru *vdev, struct dcmi_create_vdev_out *out);

dcmiReturn_t (*dcmi_get_device_info_func)(int card_id, int device_id, enum dcmi_main_cmd main_cmd, unsigned int sub_cmd, void *buf, unsigned int *size);

dcmiReturn_t (*dcmi_set_destroy_vdevice_func)(int card_id, int device_id, unsigned int VDevid);

dcmiReturn_t (*dcmi_get_device_type_func)(int card_id, int device_id, enum dcmi_unit_type *device_type);

dcmiReturn_t (*dcmi_get_device_health_func)(int card_id, int device_id, unsigned int *health);

dcmiReturn_t (*dcmi_get_device_utilization_rate_func)(int card_id, int device_id, int input_type, unsigned int *utilization_rate);

dcmiReturn_t (*dcmi_get_device_temperature_func)(int card_id, int device_id, int *temperature);

dcmiReturn_t (*dcmi_get_device_voltage_func)(int card_id, int device_id, unsigned int *voltage);

dcmiReturn_t (*dcmi_get_device_power_info_func)(int card_id, int device_id, int *power);

dcmiReturn_t (*dcmi_get_device_frequency_func)(int card_id, int device_id, enum dcmi_freq_type input_type, unsigned int *frequency);

dcmiReturn_t (*dcmi_get_device_memory_info_v3_func)(int card_id, int device_id, struct dcmi_get_memory_info_stru *memory_info);

dcmiReturn_t (*dcmi_get_device_hbm_info_func)(int card_id, int device_id, struct dcmi_hbm_info *hbm_info);

dcmiReturn_t (*dcmi_get_device_errorcode_v2_func)(int card_id, int device_id, int *error_count, unsigned int *error_code_list, unsigned int list_len);

dcmiReturn_t (*dcmi_get_device_chip_info_func)(int card_id, int device_id, struct dcmi_chip_info *chip_info);

dcmiReturn_t (*dcmi_get_device_phyid_from_logicid_func)(unsigned int logicid, unsigned int *phyid);

dcmiReturn_t (*dcmi_get_device_logicid_from_phyid_func)(unsigned int phyid, unsigned int *logicid);

dcmiReturn_t (*dcmi_get_device_ip_func)(int card_id, int device_id, enum dcmi_port_type input_type, int port_id, struct dcmi_ip_addr *ip, struct dcmi_ip_addr *mask);

dcmiReturn_t (*dcmi_get_device_network_health_func)(int card_id, int device_id, enum dcmi_rdfx_detect_result *result);

dcmiReturn_t (*dcmi_get_card_list_func)(int *card_num, int *card_list, int list_len);

dcmiReturn_t (*dcmi_get_device_id_in_card_func)(int card_id, int *device_id_max, int *mcu_id, int *cpu_id);

dcmiReturn_t (*dcmi_get_memory_info_func)(int card_id, int device_id, struct dcmi_memory_info_stru *device_memory_info);

dcmiReturn_t (*dcmi_get_device_errorcode_func)(int card_id, int device_id, int *error_count, unsigned int *error_code, int *error_width);

dcmiReturn_t (*dcmi_get_card_id_device_id_from_logicid_func)(int *card_id, int *device_id, unsigned int device_logic_id);

dcmiReturn_t (*dcmi_mcu_get_power_info_func)(int card_id, int *power);

dcmiReturn_t (*dcmi_get_product_type_func)(int card_id, int device_id, char *product_type_str, int buf_size);

dcmiReturn_t (*dcmi_set_device_reset_func)(int card_id, int device_id, enum dcmi_reset_channel channel_type);

dcmiReturn_t (*dcmi_get_device_boot_status_func)(int card_id, int device_id, enum dcmi_boot_status *boot_status);

dcmiReturn_t (*dcmi_subscribe_fault_event_func)(int card_id, int device_id, struct dcmi_event_filter filter, void (*f_name)(struct dcmi_event *fault_event));

dcmiReturn_t (*dcmi_get_npu_work_mode_func)(int card_id, unsigned char *work_mode);

dcmiReturn_t (*dcmi_get_device_die_v2_func)(int card_id, int device_id, enum dcmi_die_type input_type, struct dcmi_die_id *die_id);

dcmiReturn_t (*dcmi_get_device_resource_info_func)(int card_id, int device_id, struct dcmi_proc_mem_info *proc_info, int *proc_num);

dcmiReturn_t (*dcmi_get_device_pcie_info_v2_func)(int card_id, int device_id, struct dcmi_pcie_info_all *pcie_info);

dcmiReturn_t (*dcmi_get_device_board_info_func)(int card_id, int device_id, struct dcmi_board_info *board_info);
*/
static int last_dcmi_return_status = DCMI_SUCCESS;
static void *libascend_dcmi_handle;
static const char *unknown_error = "unknown Ascend DCMI error";
static const char *local_error_string = "";

struct gpu_info_ascend {
  struct gpu_info base;
  struct list_head allocate_list;
};

static LIST_HEAD(allocations);

static bool gpuinfo_ascend_init(void);
static void gpuinfo_ascend_shutdown(void);
static const char *gpuinfo_ascend_last_error_string(void);
static bool gpuinfo_ascend_get_device_handles(struct list_head *devices, unsigned *count);
static void gpuinfo_ascend_populate_static_info(struct gpu_info *_gpu_info);
static void gpuinfo_ascend_refresh_dynamic_info(struct gpu_info *_gpu_info);
static void gpuinfo_ascend_get_running_processes(struct gpu_info *_gpu_info);

static void _encode_card_device_id_to_pdev(char *pdev, int card_id, int device_id);
static void _decode_card_device_id_from_pdev(const char *pdev, int *card_id, int *device_id);

struct gpu_vendor gpu_vendor_ascend = {
    .init = gpuinfo_ascend_init,
    .shutdown = gpuinfo_ascend_shutdown,
    .last_error_string = gpuinfo_ascend_last_error_string,
    .get_device_handles = gpuinfo_ascend_get_device_handles,
    .populate_static_info = gpuinfo_ascend_populate_static_info,
    .refresh_dynamic_info = gpuinfo_ascend_refresh_dynamic_info,
    .refresh_running_processes = gpuinfo_ascend_get_running_processes,
    .name = "ASCEND",
};

__attribute__((constructor)) static void init_extract_gpuinfo_ascend(void) { register_gpu_vendor(&gpu_vendor_ascend); }

static bool gpuinfo_ascend_init(void) {
  /*
  libascend_dcmi_handle = dlopen("libdcmi.so", RTLD_LAZY);
  if (!libascend_dcmi_handle) {
    // local_error_string = dlerror();
    return false;
  }

  dcmi_init_func = dlsym(libascend_dcmi_handle, "dcmi_init");
  if (!dcmi_init_func) goto init_error_clean_exit;

  dcmi_get_card_num_list_func = dlsym(libascend_dcmi_handle, "dcmi_get_card_num_list");
  if (!dcmi_get_card_num_list_func) goto init_error_clean_exit;

  dcmi_get_device_num_in_card_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_num_in_card");
  if (!dcmi_get_device_num_in_card_func) goto init_error_clean_exit;

  dcmi_get_device_logic_id_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_logic_id");
  if (!dcmi_get_device_logic_id_func) goto init_error_clean_exit;

  dcmi_create_vdevice_func = dlsym(libascend_dcmi_handle, "dcmi_create_vdevice");
  if (!dcmi_create_vdevice_func) goto init_error_clean_exit;

  dcmi_get_device_info_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_info");
  if (!dcmi_get_device_info_func) goto init_error_clean_exit;

  dcmi_set_destroy_vdevice_func = dlsym(libascend_dcmi_handle, "dcmi_set_destroy_vdevice");
  if (!dcmi_set_destroy_vdevice_func) goto init_error_clean_exit;

  dcmi_get_device_type_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_type");
  if (!dcmi_get_device_type_func) goto init_error_clean_exit;

  dcmi_get_device_health_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_health");
  if (!dcmi_get_device_health_func) goto init_error_clean_exit;

  dcmi_get_device_utilization_rate_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_utilization_rate");
  if (!dcmi_get_device_utilization_rate_func) goto init_error_clean_exit;

  dcmi_get_device_temperature_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_temperature");
  if (!dcmi_get_device_temperature_func) goto init_error_clean_exit;

  dcmi_get_device_voltage_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_voltage");
  if (!dcmi_get_device_voltage_func) goto init_error_clean_exit;

  dcmi_get_device_power_info_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_power_info");
  if (!dcmi_get_device_power_info_func) goto init_error_clean_exit;

  dcmi_get_device_frequency_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_frequency");
  if (!dcmi_get_device_frequency_func) goto init_error_clean_exit;

  dcmi_get_device_memory_info_v3_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_memory_info_v3");
  if (!dcmi_get_device_memory_info_v3_func) goto init_error_clean_exit;

  dcmi_get_device_hbm_info_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_hbm_info");
  if (!dcmi_get_device_hbm_info_func) goto init_error_clean_exit;

  dcmi_get_device_errorcode_v2_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_errorcode_v2");
  if (!dcmi_get_device_errorcode_v2_func) goto init_error_clean_exit;

  dcmi_get_device_chip_info_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_chip_info");
  if (!dcmi_get_device_chip_info_func) goto init_error_clean_exit;

  dcmi_get_device_phyid_from_logicid_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_phyid_from_logicid");
  if (!dcmi_get_device_phyid_from_logicid_func) goto init_error_clean_exit;

  dcmi_get_device_logicid_from_phyid_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_logicid_from_phyid");
  if (!dcmi_get_device_logicid_from_phyid_func) goto init_error_clean_exit;

  dcmi_get_device_ip_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_ip");
  if (!dcmi_get_device_ip_func) goto init_error_clean_exit;

  dcmi_get_device_network_health_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_network_health");
  if (!dcmi_get_device_network_health_func) goto init_error_clean_exit;

  dcmi_get_card_list_func = dlsym(libascend_dcmi_handle, "dcmi_get_card_list");
  if (!dcmi_get_card_list_func) goto init_error_clean_exit;

  dcmi_get_device_id_in_card_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_id_in_card");
  if (!dcmi_get_device_id_in_card_func) goto init_error_clean_exit;

  dcmi_get_memory_info_func = dlsym(libascend_dcmi_handle, "dcmi_get_memory_info");
  if (!dcmi_get_memory_info_func) goto init_error_clean_exit;

  dcmi_get_device_errorcode_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_errorcode");
  if (!dcmi_get_device_errorcode_func) goto init_error_clean_exit;

  dcmi_get_card_id_device_id_from_logicid_func = dlsym(libascend_dcmi_handle, "dcmi_get_card_id_device_id_from_logicid");
  if (!dcmi_get_card_id_device_id_from_logicid_func) goto init_error_clean_exit;

  dcmi_mcu_get_power_info_func = dlsym(libascend_dcmi_handle, "dcmi_mcu_get_power_info");
  if (!dcmi_mcu_get_power_info_func) goto init_error_clean_exit;

  dcmi_get_product_type_func = dlsym(libascend_dcmi_handle, "dcmi_get_product_type");
  if (!dcmi_get_product_type_func) goto init_error_clean_exit;

  dcmi_set_device_reset_func = dlsym(libascend_dcmi_handle, "dcmi_set_device_reset");
  if (!dcmi_set_device_reset_func) goto init_error_clean_exit;

  dcmi_get_device_boot_status_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_boot_status");
  if (!dcmi_get_device_boot_status_func) goto init_error_clean_exit;

  dcmi_subscribe_fault_event_func = dlsym(libascend_dcmi_handle, "dcmi_subscribe_fault_event");
  if (!dcmi_subscribe_fault_event_func) goto init_error_clean_exit;

  dcmi_get_npu_work_mode_func = dlsym(libascend_dcmi_handle, "dcmi_get_npu_work_mode");
  if (!dcmi_get_npu_work_mode_func) goto init_error_clean_exit;

  dcmi_get_device_die_v2_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_die_v2");
  if (!dcmi_get_device_die_v2_func) goto init_error_clean_exit;

  dcmi_get_device_resource_info_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_resource_info");
  if (!dcmi_get_device_resource_info_func) goto init_error_clean_exit;

  dcmi_get_device_pcie_info_v2_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_pcie_info_v2");
  if (!dcmi_get_device_pcie_info_v2_func) goto init_error_clean_exit;

  dcmi_get_device_board_info_func = dlsym(libascend_dcmi_handle, "dcmi_get_device_board_info");
  if (!dcmi_get_device_board_info_func) goto init_error_clean_exit;
  */

  last_dcmi_return_status = dcmi_init();
  return last_dcmi_return_status == DCMI_SUCCESS;
  /*
init_error_clean_exit:
  dlclose(libascend_dcmi_handle);
  libascend_dcmi_handle = NULL;
  return false;
  */
}

static void gpuinfo_ascend_shutdown(void) {
  if (libascend_dcmi_handle) {
    dlclose(libascend_dcmi_handle);
    libascend_dcmi_handle = NULL;
  }
  local_error_string = "";

  struct gpu_info_ascend *allocated, *tmp;
  list_for_each_entry_safe(allocated, tmp, &allocations, allocate_list) {
    list_del(&allocated->allocate_list);
    free(allocated);
  }
}

static const char *gpuinfo_ascend_last_error_string(void) {
  return local_error_string;
}

static bool gpuinfo_ascend_get_device_handles(struct list_head *devices, unsigned *count) {
  if (!libascend_dcmi_handle) {
    return false;
  }

  int num_cards;
  int card_list[MAX_CARD_NUM] = {0};
  last_dcmi_return_status = dcmi_get_card_list(&num_cards, card_list, MAX_DEVICE_NUM);
  if (last_dcmi_return_status != DCMI_SUCCESS) {
    local_error_string = "Failed to get card num";
    return false;
  } else if (num_cards == 0) {
    local_error_string = "Not found NPU(s)";
    return false;
  }

  int num_devices = 0;
  int card_device_list[num_cards];
  for (int i = 0; i < num_cards; ++i) {
    int num_card_devices;
    last_dcmi_return_status = dcmi_get_device_num_in_card(card_list[i], &num_card_devices);
    if (last_dcmi_return_status != DCMI_SUCCESS) {
      local_error_string = "Failed to get device num of card";
      return false;
    }
    num_devices += num_card_devices;
    card_device_list[i] = num_card_devices;
  }

  struct gpu_info_ascend *gpu_infos = calloc(num_devices, sizeof(*gpu_infos));
  if (!gpu_infos) {
    local_error_string = strerror(errno);
    return false;
  }

  // todo: for free gpu_infos when shutting down, rewrite to direct free?
  list_add(&gpu_infos[0].allocate_list, &allocations);

  *count = 0;
  for (int i = 0; i < num_cards; ++i) {
    for (int j = 0; j < card_device_list[i]; ++j) {
      gpu_infos[*count].base.vendor = &gpu_vendor_ascend;
      _encode_card_device_id_to_pdev(gpu_infos[*count].base.pdev, i, j);
      list_add_tail(&gpu_infos[*count].base.list, devices);
      *count += 1;
    }
  }

  return true;
}

static void _encode_card_device_id_to_pdev(char *pdev, int card_id, int device_id) {
  sprintf(pdev, "%d-%d", card_id, device_id);
}

static void _decode_card_device_id_from_pdev(const char *pdev, int *card_id, int *device_id) {
  sscanf(pdev, "%d-%d", card_id, device_id);
}

static void gpuinfo_ascend_populate_static_info(struct gpu_info *_gpu_info) {
  struct gpu_info_ascend *gpu_info = container_of(_gpu_info, struct gpu_info_ascend, base);
  struct gpuinfo_static_info *static_info = &gpu_info->base.static_info;
  static_info->integrated_graphics = false;
  RESET_ALL(static_info->valid);

  int card_id, device_id;
  _decode_card_device_id_from_pdev(_gpu_info->pdev, &card_id, &device_id);

  struct dcmi_chip_info *chip_info = malloc(sizeof(struct dcmi_chip_info));
  last_dcmi_return_status = dcmi_get_device_chip_info(card_id, device_id, chip_info);
  if (last_dcmi_return_status == DCMI_SUCCESS) {
    // assume Ascend only use ASCII code for chip name
    strcpy(static_info->device_name, (char *) chip_info->chip_name);
    SET_VALID(gpuinfo_device_name_valid, static_info->valid);
  }
  free(chip_info);
  // todo: it seems that other static infos are not supported by Ascend DCMI for now, will add if possible in future
}

static void gpuinfo_ascend_refresh_dynamic_info(struct gpu_info *_gpu_info) {
  struct gpu_info_ascend *gpu_info = container_of(_gpu_info, struct gpu_info_ascend, base);
  struct gpuinfo_dynamic_info *dynamic_info = &gpu_info->base.dynamic_info;
  RESET_ALL(dynamic_info->valid);
  dynamic_info->encode_decode_shared = false;

  int card_id, device_id;
  _decode_card_device_id_from_pdev(_gpu_info->pdev, &card_id, &device_id);

  unsigned aicore_freq;
  last_dcmi_return_status = dcmi_get_device_frequency(card_id, device_id, DCMI_FREQ_AICORE_CURRENT_, &aicore_freq);
  if (last_dcmi_return_status == DCMI_SUCCESS) {
    dynamic_info->gpu_clock_speed = aicore_freq;
    SET_VALID(gpuinfo_gpu_clock_speed_valid, dynamic_info->valid);
  }

  unsigned aicore_max_freq;
  last_dcmi_return_status = dcmi_get_device_frequency(card_id, device_id, DCMI_FREQ_AICORE_MAX, &aicore_max_freq);
  if (last_dcmi_return_status == DCMI_SUCCESS) {
    dynamic_info->gpu_clock_speed_max = aicore_max_freq;
    SET_VALID(gpuinfo_gpu_clock_speed_max_valid, dynamic_info->valid);
  }

  unsigned hbm_freq;
  last_dcmi_return_status = dcmi_get_device_frequency(card_id, device_id, DCMI_FREQ_HBM, &hbm_freq);
  if (last_dcmi_return_status == DCMI_SUCCESS) {
    dynamic_info->mem_clock_speed = hbm_freq;
    SET_VALID(gpuinfo_mem_clock_speed_valid, dynamic_info->valid);
  }

  unsigned aicore_util_rate;
  last_dcmi_return_status = dcmi_get_device_utilization_rate(card_id, device_id, DCMI_UTILIZATION_RATE_AICORE, &aicore_util_rate);
  if (last_dcmi_return_status == DCMI_SUCCESS) {
    dynamic_info->gpu_util_rate = aicore_util_rate;
    SET_VALID(gpuinfo_gpu_util_rate_valid, dynamic_info->valid);
  }

  struct dsmi_hbm_info_stru *hbm_info = malloc(sizeof(struct dsmi_hbm_info_stru));
  last_dcmi_return_status = dcmi_get_hbm_info(card_id, device_id, hbm_info);
  if (last_dcmi_return_status == DCMI_SUCCESS) {
    SET_GPUINFO_DYNAMIC(dynamic_info, total_memory, hbm_info->memory_size);
    SET_GPUINFO_DYNAMIC(dynamic_info, used_memory, hbm_info->memory_usage);
    SET_GPUINFO_DYNAMIC(dynamic_info, free_memory, hbm_info->memory_size - hbm_info->memory_usage);
    SET_GPUINFO_DYNAMIC(dynamic_info, mem_util_rate, hbm_info->memory_usage * 100 / hbm_info->memory_size);
  }
  free(hbm_info);

  int device_temperature;
  last_dcmi_return_status = dcmi_get_device_temperature(card_id, device_id, &device_temperature);
  if (last_dcmi_return_status == DCMI_SUCCESS) {
    dynamic_info->gpu_temp = device_temperature;
    SET_VALID(gpuinfo_gpu_temp_valid, dynamic_info->valid);
  }

  int power_usage;
  last_dcmi_return_status = dcmi_get_device_power_info(card_id, device_id, &power_usage);
  if (last_dcmi_return_status == DCMI_SUCCESS) {
    dynamic_info->power_draw = power_usage * 100;
    SET_VALID(gpuinfo_power_draw_valid, dynamic_info->valid);
  }
}

static void gpuinfo_ascend_get_running_processes(struct gpu_info *_gpu_info) {
  int card_id, device_id;
  _decode_card_device_id_from_pdev(_gpu_info->pdev, &card_id, &device_id);

  struct dcmi_proc_mem_info *proc_info = malloc(MAX_PROC_NUM * sizeof(struct dcmi_proc_mem_info));
  int proc_num = 0;
  last_dcmi_return_status = dcmi_get_device_resource_info(card_id, device_id, proc_info, &proc_num);
  if (last_dcmi_return_status == DCMI_SUCCESS) {
    _gpu_info->processes_count = proc_num;
    _gpu_info->processes_array_size = proc_num + PROC_ALLOC_INC;
    _gpu_info->processes = reallocarray(_gpu_info->processes, _gpu_info->processes_array_size, sizeof(*_gpu_info->processes));
    if (!_gpu_info->processes) {
      perror("Could not allocate memory: ");
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < proc_num; i++) {
      _gpu_info->processes[i].type = gpu_process_compute;
      _gpu_info->processes[i].pid = proc_info[i].proc_id;
      _gpu_info->processes[i].gpu_memory_usage = proc_info[i].proc_mem_usage;
      SET_VALID(gpuinfo_process_gpu_memory_usage_valid, _gpu_info->processes[i].valid);
    }
  }
  free(proc_info);
}