/*
 *  Copyright (C) 2017. Mellanox Technologies, Ltd. ALL RIGHTS RESERVED.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may
 *    not use this file except in compliance with the License. You may obtain
 *    a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 *    THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 *    CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *    LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 *    FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 *    See the Apache Version 2.0 License for specific language governing
 *    permissions and limitations under the License.
 *
 */

#include "sai_windows.h"
#include "sai.h"
#include "mlnx_sai.h"
#include "assert.h"

#undef  __MODULE__
#define __MODULE__ SAI_QUEUE

static sx_verbosity_level_t LOG_VAR_NAME(__MODULE__) = SX_VERBOSITY_LEVEL_WARNING;

typedef sai_status_t (*queue_profile_setter_t) (sai_object_id_t profile_id, sai_object_id_t queue_id);

static sai_status_t mlnx_queue_config_set(_In_ const sai_object_key_t      *key,
                                          _In_ const sai_attribute_value_t *value,
                                          void                             *arg);
static sai_status_t mlnx_queue_config_get(_In_ const sai_object_key_t   *key,
                                          _Inout_ sai_attribute_value_t *value,
                                          _In_ uint32_t                  attr_index,
                                          _Inout_ vendor_cache_t        *cache,
                                          void                          *arg);
static sai_status_t mlnx_queue_type_get(_In_ const sai_object_key_t   *key,
                                        _Inout_ sai_attribute_value_t *value,
                                        _In_ uint32_t                  attr_index,
                                        _Inout_ vendor_cache_t        *cache,
                                        void                          *arg);
static sai_status_t mlnx_queue_index_get(_In_ const sai_object_key_t   *key,
                                         _Inout_ sai_attribute_value_t *value,
                                         _In_ uint32_t                  attr_index,
                                         _Inout_ vendor_cache_t        *cache,
                                         void                          *arg);
static sai_status_t mlnx_queue_port_get(_In_ const sai_object_key_t   *key,
                                        _Inout_ sai_attribute_value_t *value,
                                        _In_ uint32_t                  attr_index,
                                        _Inout_ vendor_cache_t        *cache,
                                        void                          *arg);
static sai_status_t mlnx_queue_parent_sched_node_get(_In_ const sai_object_key_t   *key,
                                                     _Inout_ sai_attribute_value_t *value,
                                                     _In_ uint32_t                  attr_index,
                                                     _Inout_ vendor_cache_t        *cache,
                                                     void                          *arg);
static sai_status_t mlnx_queue_parent_sched_node_set(_In_ const sai_object_key_t      *key,
                                                     _In_ const sai_attribute_value_t *value,
                                                     void                             *arg);
static const sai_attribute_entry_t        queue_attribs[] = {
    { SAI_QUEUE_ATTR_TYPE, true, true, false, true,
      "Queue type", SAI_ATTR_VAL_TYPE_S32 },
    { SAI_QUEUE_ATTR_PORT, true, true, false, true,
      "Port ID", SAI_ATTR_VAL_TYPE_OID },
    { SAI_QUEUE_ATTR_INDEX, true, true, false, true,
      "Queue index", SAI_ATTR_VAL_TYPE_U8 },
    { SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE, true, true, true, true,
      "Parent scheduler node ID", SAI_ATTR_VAL_TYPE_OID },
    { SAI_QUEUE_ATTR_WRED_PROFILE_ID, false, true, true, true,
      "Queue WRED profile ID", SAI_ATTR_VAL_TYPE_OID },
    { SAI_QUEUE_ATTR_BUFFER_PROFILE_ID, false, true, true, true,
      "Queue buffer profile ID", SAI_ATTR_VAL_TYPE_OID },
    { SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID, false, true, true, true,
      "Queue scheduler profile ID", SAI_ATTR_VAL_TYPE_OID },
    { SAI_QUEUE_ATTR_PAUSE_STATUS, false, false, false, true,
      "Queue pause status", SAI_ATTR_VAL_TYPE_BOOL },
    { END_FUNCTIONALITY_ATTRIBS_ID, false, false, false, false,
      "", SAI_ATTR_VAL_TYPE_UNDETERMINED }
};
static const sai_vendor_attribute_entry_t queue_vendor_attribs[] = {
    { SAI_QUEUE_ATTR_TYPE,
      { true, false, false, true },
      { true, false, false, true },
      mlnx_queue_type_get, NULL,
      NULL, NULL },
    { SAI_QUEUE_ATTR_PORT,
      { true, false, false, true },
      { true, false, false, true },
      mlnx_queue_port_get, NULL,
      NULL, NULL },
    { SAI_QUEUE_ATTR_INDEX,
      { true, false, false, true },
      { true, false, false, true },
      mlnx_queue_index_get, NULL,
      NULL, NULL },
    { SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE,
      { true, false, true, true },
      { true, false, true, true },
      mlnx_queue_parent_sched_node_get, NULL,
      mlnx_queue_parent_sched_node_set, NULL },
    { SAI_QUEUE_ATTR_WRED_PROFILE_ID,
      { true, false, true, true },
      { true, false, true, true },
      mlnx_queue_config_get, (void*)SAI_QUEUE_ATTR_WRED_PROFILE_ID,
      mlnx_queue_config_set, (void*)SAI_QUEUE_ATTR_WRED_PROFILE_ID },
    { SAI_QUEUE_ATTR_BUFFER_PROFILE_ID,
      { true, false, true, true },
      { true, false, true, true },
      mlnx_queue_config_get, (void*)SAI_QUEUE_ATTR_BUFFER_PROFILE_ID,
      mlnx_queue_config_set, (void*)SAI_QUEUE_ATTR_BUFFER_PROFILE_ID },
    { SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID,
      { true, false, true, true },
      { true, false, true, true },
      mlnx_queue_config_get, (void*)SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID,
      mlnx_queue_config_set, (void*)SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID },
    { SAI_QUEUE_ATTR_PAUSE_STATUS,
      { false, false, false, false },
      { false, false, false, true },
      NULL, NULL,
      NULL, NULL },
};

sai_status_t mlnx_queue_log_set(sx_verbosity_level_t level)
{
    LOG_VAR_NAME(__MODULE__) = level;

    if (gh_sdk) {
        return sdk_to_sai(sx_api_cos_log_verbosity_level_set(gh_sdk, SX_LOG_VERBOSITY_BOTH, level, level));
    } else {
        return SAI_STATUS_SUCCESS;
    }
}

static void queue_key_to_str(_In_ sai_object_id_t queue_id, _Out_ char *key_str)
{
    uint32_t port_num;
    uint8_t  ext_data[EXTENDED_DATA_SIZE] = {0};

    if (SAI_STATUS_SUCCESS != mlnx_object_to_type(queue_id, SAI_OBJECT_TYPE_QUEUE, &port_num, ext_data)) {
        snprintf(key_str, MAX_KEY_STR_LEN, "invalid queue");
    } else {
        snprintf(key_str, MAX_KEY_STR_LEN, "queue %u:%u", port_num, ext_data[0]);
    }
}

/* Set queue buffer and scheduler profiles */
static sai_status_t mlnx_queue_config_set(_In_ const sai_object_key_t      *key,
                                          _In_ const sai_attribute_value_t *value,
                                          void                             *arg)
{
    sai_object_id_t        queue_id                     = key->key.object_id;
    sai_object_id_t        profile_id                   = value->oid;
    uint8_t                ext_data[EXTENDED_DATA_SIZE] = {0};
    sx_port_log_id_t       port_num;
    uint32_t               queue_num    = 0, profile_num = 0;
    long                   attr         = (long)arg;
    sai_object_type_t      profile_type = SAI_NULL_OBJECT_ID;
    sai_status_t           status       = SAI_STATUS_SUCCESS;
    queue_profile_setter_t func_setter  = NULL;

    SX_LOG_ENTER();
    assert((SAI_QUEUE_ATTR_BUFFER_PROFILE_ID == attr) ||
           (SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID == attr) ||
           (SAI_QUEUE_ATTR_WRED_PROFILE_ID == attr));

    if (SAI_STATUS_SUCCESS != mlnx_object_to_type(queue_id, SAI_OBJECT_TYPE_QUEUE, &port_num, ext_data)) {
        return SAI_STATUS_INVALID_PARAMETER;
    }
    queue_num = ext_data[0];
    if (queue_num > g_resource_limits.cos_port_ets_traffic_class_max) {
        SX_LOG_ERR("Invalid queue num %u - exceed maximum %u\n", queue_num,
                   g_resource_limits.cos_port_ets_traffic_class_max);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    switch (attr) {
    case SAI_QUEUE_ATTR_WRED_PROFILE_ID:
        profile_type = SAI_OBJECT_TYPE_WRED;
        func_setter  = mlnx_wred_apply;
        break;

    case SAI_QUEUE_ATTR_BUFFER_PROFILE_ID:
        profile_type = SAI_OBJECT_TYPE_BUFFER_PROFILE;
        func_setter  = mlnx_buffer_apply;
        break;

    case SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID:
        profile_type = SAI_OBJECT_TYPE_SCHEDULER;
        func_setter  = mlnx_scheduler_to_queue_apply;
        break;
    }

    if (SAI_NULL_OBJECT_ID != profile_id) {
        if (SAI_STATUS_SUCCESS != mlnx_object_to_type(profile_id, profile_type, &profile_num, NULL)) {
            SX_LOG_ERR("Failed to set profile for queue - Invalid object id\n");
            return SAI_STATUS_INVALID_PARAMETER;
        }
    }

    sai_db_write_lock();
    status = func_setter(profile_id, queue_id);
    sai_db_unlock();
    SX_LOG_EXIT();
    return status;
}

static sai_status_t mlnx_queue_config_get(_In_ const sai_object_key_t   *key,
                                          _Inout_ sai_attribute_value_t *value,
                                          _In_ uint32_t                  attr_index,
                                          _Inout_ vendor_cache_t        *cache,
                                          void                          *arg)
{
    sai_object_id_t          queue_id                     = key->key.object_id;
    uint8_t                  ext_data[EXTENDED_DATA_SIZE] = {0};
    sx_port_log_id_t         port_num;
    uint32_t                 queue_num = 0;
    long                     attr      = (long)arg;
    sai_status_t             status;
    mlnx_qos_queue_config_t *queue_cfg;
    uint32_t                 buffer_db_index;

    SX_LOG_ENTER();

    assert((SAI_QUEUE_ATTR_WRED_PROFILE_ID == attr) ||
           (SAI_QUEUE_ATTR_BUFFER_PROFILE_ID == attr) ||
           (SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID == attr));

    status = mlnx_object_to_type(queue_id, SAI_OBJECT_TYPE_QUEUE, &port_num, ext_data);
    if (status != SAI_STATUS_SUCCESS) {
        return status;
    }
    queue_num = ext_data[0];

    sai_db_read_lock();

    status = mlnx_queue_cfg_lookup(port_num, queue_num, &queue_cfg);
    if (status != SAI_STATUS_SUCCESS) {
        goto out;
    }

    switch (attr) {
    case SAI_QUEUE_ATTR_WRED_PROFILE_ID:
        value->oid = queue_cfg->wred_id;
        break;

    case SAI_QUEUE_ATTR_BUFFER_PROFILE_ID:
        if (SAI_STATUS_SUCCESS == (status = get_buffer_profile_db_index(queue_cfg->buffer_id, &buffer_db_index))) {
            value->oid = queue_cfg->buffer_id;
        }
        break;

    case SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID:
        value->oid = queue_cfg->sched_obj.scheduler_id;
        break;
    }

out:
    sai_db_unlock();
    SX_LOG_EXIT();
    return status;
}

/**
 * @brief Queue type
 *
 * @type sai_queue_type_t
 * @flags MANDATORY_ON_CREATE | CREATE_ONLY | KEY
 */
static sai_status_t mlnx_queue_type_get(_In_ const sai_object_key_t   *key,
                                        _Inout_ sai_attribute_value_t *value,
                                        _In_ uint32_t                  attr_index,
                                        _Inout_ vendor_cache_t        *cache,
                                        void                          *arg)
{
    sai_object_id_t  queue_id                     = key->key.object_id;
    uint8_t          ext_data[EXTENDED_DATA_SIZE] = {0};
    sx_port_log_id_t port_num;
    uint32_t         queue_num = 0;
    boolean_t        mc_aware  = false;
    sx_status_t      sx_status = SX_STATUS_SUCCESS;

    SX_LOG_ENTER();

    if (SAI_STATUS_SUCCESS != mlnx_object_to_type(queue_id, SAI_OBJECT_TYPE_QUEUE, &port_num, ext_data)) {
        return SAI_STATUS_INVALID_PARAMETER;
    }
    queue_num = ext_data[0];
    if (queue_num > g_resource_limits.cos_port_ets_traffic_class_max) {
        SX_LOG_ERR("Invalid queue num %u - exceed maximum %u\n", queue_num,
                   g_resource_limits.cos_port_ets_traffic_class_max);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    sx_status = sx_api_cos_port_tc_mcaware_get(gh_sdk, port_num, &mc_aware);
    if (SX_STATUS_SUCCESS != sx_status) {
        SX_LOG_ERR("Failed to get MC status for the port 0x%x - %s\n", port_num, SX_STATUS_MSG(sx_status));
        return sdk_to_sai(sx_status);
    }

    if (false == mc_aware) {
        value->s32 = SAI_QUEUE_TYPE_ALL;
    } else {
        value->s32 = (queue_num < ((g_resource_limits.cos_port_ets_traffic_class_max + 1) / 2)) ?
                     SAI_QUEUE_TYPE_UNICAST : SAI_QUEUE_TYPE_MULTICAST;
    }

    SX_LOG_EXIT();
    return SAI_STATUS_SUCCESS;
}

/**
 * @brief Queue index
 *
 * @type sai_uint8_t
 * @flags MANDATORY_ON_CREATE | CREATE_ONLY | KEY
 */
static sai_status_t mlnx_queue_index_get(_In_ const sai_object_key_t   *key,
                                         _Inout_ sai_attribute_value_t *value,
                                         _In_ uint32_t                  attr_index,
                                         _Inout_ vendor_cache_t        *cache,
                                         void                          *arg)
{
    uint8_t          ext_data[EXTENDED_DATA_SIZE] = {0};
    sx_port_log_id_t port_num;
    sai_status_t     status;

    SX_LOG_ENTER();

    status = mlnx_object_to_type(key->key.object_id, SAI_OBJECT_TYPE_QUEUE, &port_num, ext_data);
    if (SAI_ERR(status)) {
        return status;
    }

    value->u8 = ext_data[0];

    SX_LOG_EXIT();
    return SAI_STATUS_SUCCESS;
}

/**
 * @brief Pord id
 *
 * @type sai_object_id_t
 * @objects SAI_OBJECT_TYPE_PORT
 * @flags MANDATORY_ON_CREATE | CREATE_ONLY | KEY
 */
static sai_status_t mlnx_queue_port_get(_In_ const sai_object_key_t   *key,
                                        _Inout_ sai_attribute_value_t *value,
                                        _In_ uint32_t                  attr_index,
                                        _Inout_ vendor_cache_t        *cache,
                                        void                          *arg)
{
    uint8_t          ext_data[EXTENDED_DATA_SIZE] = {0};
    sx_port_log_id_t port_log_id;
    sai_status_t     status;

    SX_LOG_ENTER();

    status = mlnx_object_to_type(key->key.object_id, SAI_OBJECT_TYPE_QUEUE, &port_log_id, ext_data);
    if (SAI_ERR(status)) {
        return status;
    }

    status = mlnx_create_object(SAI_OBJECT_TYPE_PORT, port_log_id, NULL, &value->oid);

    SX_LOG_EXIT();
    return status;
}

/**
 * @brief Parent scheduler node
 *
 * In case of Hierarchical Qos not supported, the parent node is the port.
 * Condition on whether Hierarchial Qos is supported or not, need to remove
 * the MANDATORY_ON_CREATE FLAG when HQoS is introduced
 *
 * @type sai_object_id_t
 * @objects SAI_OBJECT_TYPE_SCHEDULER_GROUP, SAI_OBJECT_TYPE_PORT
 * @flags MANDATORY_ON_CREATE | CREATE_AND_SET
 */
static sai_status_t mlnx_queue_parent_sched_node_get(_In_ const sai_object_key_t   *key,
                                                     _Inout_ sai_attribute_value_t *value,
                                                     _In_ uint32_t                  attr_index,
                                                     _Inout_ vendor_cache_t        *cache,
                                                     void                          *arg)
{
    return mlnx_sched_group_parent_get(key, value, attr_index, cache, arg);
}

/**
 * @brief Parent scheduler node
 *
 * In case of Hierarchical Qos not supported, the parent node is the port.
 * Condition on whether Hierarchial Qos is supported or not, need to remove
 * the MANDATORY_ON_CREATE FLAG when HQoS is introduced
 *
 * @type sai_object_id_t
 * @objects SAI_OBJECT_TYPE_SCHEDULER_GROUP, SAI_OBJECT_TYPE_PORT
 * @flags MANDATORY_ON_CREATE | CREATE_AND_SET
 */
static sai_status_t mlnx_queue_parent_sched_node_set(_In_ const sai_object_key_t      *key,
                                                     _In_ const sai_attribute_value_t *value,
                                                     void                             *arg)
{
    return mlnx_sched_group_parent_set(key, value, arg);
}

/*
 * Routine Description:
 *   Set queue attribute value.
 *
 * Arguments:
 *    [in] queue_id - queue id
 *    [in] attr - attribute
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t mlnx_set_queue_attribute(_In_ sai_object_id_t queue_id, _In_ const sai_attribute_t *attr)
{
    sai_status_t           sai_status;
    const sai_object_key_t key                      = { .key.object_id = queue_id };
    char                   key_str[MAX_KEY_STR_LEN] = {0};

    SX_LOG_ENTER();

    queue_key_to_str(queue_id, key_str);
    sai_status = sai_set_attribute(&key, key_str, queue_attribs, queue_vendor_attribs, attr);
    SX_LOG_EXIT();
    return sai_status;
}

/*
 * Routine Description:
 *   Get queue attribute value.
 *
 * Arguments:
 *    [in] queue_id - queue id
 *    [in] attr_count - number of attributes
 *    [inout] attr_list - array of attributes
 *
 * Return Values:
 *    SAI_STATUS_SUCCESS on success
 *    Failure status code on error
 */
static sai_status_t mlnx_get_queue_attribute(_In_ sai_object_id_t     queue_id,
                                             _In_ uint32_t            attr_count,
                                             _Inout_ sai_attribute_t *attr_list)
{
    sai_status_t           sai_status;
    const sai_object_key_t key                      = { .key.object_id = queue_id };
    char                   key_str[MAX_KEY_STR_LEN] = {0};

    SX_LOG_ENTER();

    queue_key_to_str(queue_id, key_str);
    sai_status = sai_get_attributes(&key, key_str, queue_attribs, queue_vendor_attribs, attr_count, attr_list);
    SX_LOG_EXIT();
    return sai_status;
}

/**
 * @brief Get queue statistics counters.
 *
 * @param[in] queue_id Queue id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 * @param[out] counters Array of resulting counter values.
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
static sai_status_t mlnx_get_queue_statistics(_In_ sai_object_id_t         queue_id,
                                              _In_ uint32_t                number_of_counters,
                                              _In_ const sai_queue_stat_t *counter_ids,
                                              _Out_ uint64_t              *counters)
{
    sai_status_t                     status;
    uint8_t                          ext_data[EXTENDED_DATA_SIZE] = {0};
    uint8_t                          queue_num                    = 0;
    sx_port_log_id_t                 port_num;
    uint32_t                         ii = 0;
    char                             key_str[MAX_KEY_STR_LEN];
    sx_port_statistic_usage_params_t stats_usage;
    sx_port_occupancy_statistics_t   occupancy_stats;
    uint32_t                         usage_cnt = 1;
    sx_port_traffic_cntr_t           tc_cnts;

    SX_LOG_ENTER();

    queue_key_to_str(queue_id, key_str);
    SX_LOG_DBG("Get queue stats %s\n", key_str);

    if (NULL == counter_ids) {
        SX_LOG_ERR("NULL counter ids array param\n");
        return SAI_STATUS_INVALID_PARAMETER;
    }

    if (NULL == counters) {
        SX_LOG_ERR("NULL counters array param\n");
        return SAI_STATUS_INVALID_PARAMETER;
    }

    if (SAI_STATUS_SUCCESS != mlnx_object_to_type(queue_id, SAI_OBJECT_TYPE_QUEUE, &port_num, ext_data)) {
        return SAI_STATUS_INVALID_PARAMETER;
    }
    queue_num = ext_data[0];
    /* TODO : change to > g_resource_limits.cos_port_ets_traffic_class_max when sdk is updated to use rm */
    if (queue_num >= RM_API_COS_TRAFFIC_CLASS_NUM) {
        SX_LOG_ERR("Invalid queue num %u - exceed maximum %u\n", queue_num,
                   g_resource_limits.cos_port_ets_traffic_class_max);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    if (SX_STATUS_SUCCESS !=
        (status = sx_api_port_counter_tc_get(gh_sdk, SX_ACCESS_CMD_READ, port_num, queue_num, &tc_cnts))) {
        SX_LOG_ERR("Failed to get port tc counters - %s.\n", SX_STATUS_MSG(status));
        return sdk_to_sai(status);
    }

    memset(&stats_usage, 0, sizeof(stats_usage));
    stats_usage.port_cnt                                 = 1;
    stats_usage.log_port_list_p                          = &port_num;
    stats_usage.sx_port_params.port_params_type          = SX_COS_EGRESS_PORT_TRAFFIC_CLASS_ATTR_E;
    stats_usage.sx_port_params.port_params_cnt           = 1;
    stats_usage.sx_port_params.port_param.port_tc_list_p = &queue_num;

    if (SX_STATUS_SUCCESS !=
        (status = sx_api_cos_port_buff_type_statistic_get(gh_sdk, SX_ACCESS_CMD_READ, &stats_usage, 1,
                                                          &occupancy_stats, &usage_cnt))) {
        SX_LOG_ERR("Failed to get port buff statistics - %s.\n", SX_STATUS_MSG(status));
        return sdk_to_sai(status);
    }

    for (ii = 0; ii < number_of_counters; ii++) {
        switch (counter_ids[ii]) {
        case SAI_QUEUE_STAT_DROPPED_BYTES:
        case SAI_QUEUE_STAT_GREEN_PACKETS:
        case SAI_QUEUE_STAT_GREEN_BYTES:
        case SAI_QUEUE_STAT_GREEN_DROPPED_PACKETS:
        case SAI_QUEUE_STAT_GREEN_DROPPED_BYTES:
        case SAI_QUEUE_STAT_YELLOW_PACKETS:
        case SAI_QUEUE_STAT_YELLOW_BYTES:
        case SAI_QUEUE_STAT_YELLOW_DROPPED_PACKETS:
        case SAI_QUEUE_STAT_YELLOW_DROPPED_BYTES:
        case SAI_QUEUE_STAT_RED_PACKETS:
        case SAI_QUEUE_STAT_RED_BYTES:
        case SAI_QUEUE_STAT_RED_DROPPED_PACKETS:
        case SAI_QUEUE_STAT_RED_DROPPED_BYTES:
        case SAI_QUEUE_STAT_GREEN_DISCARD_DROPPED_PACKETS:
        case SAI_QUEUE_STAT_GREEN_DISCARD_DROPPED_BYTES:
        case SAI_QUEUE_STAT_YELLOW_DISCARD_DROPPED_PACKETS:
        case SAI_QUEUE_STAT_YELLOW_DISCARD_DROPPED_BYTES:
        case SAI_QUEUE_STAT_RED_DISCARD_DROPPED_PACKETS:
        case SAI_QUEUE_STAT_RED_DISCARD_DROPPED_BYTES:
        case SAI_QUEUE_STAT_DISCARD_DROPPED_BYTES:
        case SAI_QUEUE_STAT_SHARED_CURR_OCCUPANCY_BYTES:
        case SAI_QUEUE_STAT_SHARED_WATERMARK_BYTES:
            SX_LOG_ERR("Queue counter %d set item %u not supported\n", counter_ids[ii], ii);
            return SAI_STATUS_ATTR_NOT_SUPPORTED_0;

        case SAI_QUEUE_STAT_PACKETS:
            counters[ii] = tc_cnts.tx_frames;
            break;

        case SAI_QUEUE_STAT_BYTES:
            counters[ii] = tc_cnts.tx_octet;
            break;

        case SAI_QUEUE_STAT_DROPPED_PACKETS:
            counters[ii] = tc_cnts.tx_no_buffer_discard_uc;
            break;

        case SAI_QUEUE_STAT_DISCARD_DROPPED_PACKETS:
            counters[ii] = tc_cnts.tx_wred_discard;
            break;

        case SAI_QUEUE_STAT_CURR_OCCUPANCY_BYTES:
            counters[ii] = (uint64_t)occupancy_stats.statistics.curr_occupancy *
                           g_resource_limits.shared_buff_buffer_unit_size;
            break;

        case SAI_QUEUE_STAT_WATERMARK_BYTES:
            counters[ii] = (uint64_t)occupancy_stats.statistics.watermark *
                           g_resource_limits.shared_buff_buffer_unit_size;
            break;

        default:
            SX_LOG_ERR("Invalid queue counter %d\n", counter_ids[ii]);
            return SAI_STATUS_INVALID_PARAMETER;
        }
    }

    SX_LOG_EXIT();
    return SAI_STATUS_SUCCESS;
}

/**
 * @brief Clear queue statistics counters.
 *
 * @param[in] queue_id Queue id
 * @param[in] number_of_counters Number of counters in the array
 * @param[in] counter_ids Specifies the array of counter ids
 *
 * @return #SAI_STATUS_SUCCESS on success Failure status code on error
 */
static sai_status_t mlnx_clear_queue_stats(_In_ sai_object_id_t         queue_id,
                                           _In_ uint32_t                number_of_counters,
                                           _In_ const sai_queue_stat_t *counter_ids)
{
    sai_status_t                     status;
    uint8_t                          ext_data[EXTENDED_DATA_SIZE] = { 0 };
    uint8_t                          queue_num                    = 0;
    sx_port_log_id_t                 port_num;
    char                             key_str[MAX_KEY_STR_LEN];
    sx_port_statistic_usage_params_t stats_usage;
    sx_port_occupancy_statistics_t   occupancy_stats;
    uint32_t                         usage_cnt = 1;
    sx_port_traffic_cntr_t           tc_cnts;

    SX_LOG_ENTER();

    queue_key_to_str(queue_id, key_str);
    SX_LOG_NTC("Clear queue stats %s\n", key_str);

    if (NULL == counter_ids) {
        SX_LOG_ERR("NULL counter ids array param\n");
        return SAI_STATUS_INVALID_PARAMETER;
    }

    if (SAI_STATUS_SUCCESS != mlnx_object_to_type(queue_id, SAI_OBJECT_TYPE_QUEUE, &port_num, ext_data)) {
        return SAI_STATUS_INVALID_PARAMETER;
    }
    queue_num = ext_data[0];
    /* TODO : change to > g_resource_limits.cos_port_ets_traffic_class_max when sdk is updated to use rm */
    if (queue_num >= RM_API_COS_TRAFFIC_CLASS_NUM) {
        SX_LOG_ERR("Invalid queue num %u - exceed maximum %u\n", queue_num,
                   g_resource_limits.cos_port_ets_traffic_class_max);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    if (SX_STATUS_SUCCESS !=
        (status = sx_api_port_counter_tc_get(gh_sdk, SX_ACCESS_CMD_READ_CLEAR, port_num, queue_num, &tc_cnts))) {
        SX_LOG_ERR("Failed to get clear port tc counters - %s.\n", SX_STATUS_MSG(status));
        return sdk_to_sai(status);
    }

    memset(&stats_usage, 0, sizeof(stats_usage));
    stats_usage.port_cnt                                 = 1;
    stats_usage.log_port_list_p                          = &port_num;
    stats_usage.sx_port_params.port_params_type          = SX_COS_EGRESS_PORT_TRAFFIC_CLASS_ATTR_E;
    stats_usage.sx_port_params.port_params_cnt           = 1;
    stats_usage.sx_port_params.port_param.port_tc_list_p = &queue_num;

    if (SX_STATUS_SUCCESS !=
        (status = sx_api_cos_port_buff_type_statistic_get(gh_sdk, SX_ACCESS_CMD_READ_CLEAR, &stats_usage, 1,
                                                          &occupancy_stats, &usage_cnt))) {
        SX_LOG_ERR("Failed to get clear port buff statistics - %s.\n", SX_STATUS_MSG(status));
        return sdk_to_sai(status);
    }

    SX_LOG_EXIT();
    return SAI_STATUS_SUCCESS;
}

/* QoS DB lock is required */
sai_status_t mlnx_queue_cfg_lookup(sx_port_log_id_t log_port_id, uint32_t queue_idx, mlnx_qos_queue_config_t **cfg)
{
    mlnx_port_config_t *port;
    uint32_t            ii;

    if (queue_idx >= MAX_QUEUES) {
        SX_LOG_ERR("Invalid queue num %u - exceed maximum %u\n", queue_idx,
                   MAX_QUEUES - 1);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    mlnx_port_foreach(port, ii) {
        if (port->logical != log_port_id) {
            continue;
        }

        *cfg = &g_sai_qos_db_ptr->queue_db[port->start_queues_index + queue_idx];
        return SAI_STATUS_SUCCESS;
    }

    SX_LOG_ERR("Filed to lookup queue by index %u on port log id %x\n",
               queue_idx, log_port_id);

    return SAI_STATUS_INVALID_PARAMETER;
}

/**
 * Routine Description:
 *    @brief Create queue
 *
 * Arguments:
 *    @param[out] queue_id - queue id
 *    @param[in] attr_count - number of attributes
 *    @param[in] attr_list - array of attributes
 *
 * Return Values:
 *    @return SAI_STATUS_SUCCESS on success
 *            Failure status code on error
 *
 */
sai_status_t mlnx_create_queue(_Out_ sai_object_id_t      *queue_id,
                               _In_ sai_object_id_t        switch_id,
                               _In_ uint32_t               attr_count,
                               _In_ const sai_attribute_t *attr_list)
{
    const sai_attribute_value_t *sched_profile_attr = NULL;
    const sai_attribute_value_t *buff_profile_attr  = NULL;
    const sai_attribute_value_t *wred_profile_attr  = NULL;
    const sai_attribute_value_t *index_attr         = NULL;
    const sai_attribute_value_t *type_attr          = NULL;
    const sai_attribute_value_t *port_attr          = NULL;
    const sai_attribute_value_t *parent_attr        = NULL;
    uint32_t                     sched_profile_idx;
    uint32_t                     buff_profile_idx;
    uint32_t                     wred_profile_idx;
    uint32_t                     index_idx;
    uint32_t                     type_idx;
    uint32_t                     port_idx;
    uint32_t                     parent_idx;
    sx_port_log_id_t             port_id;
    sai_status_t                 status;
    char                         key_str[MAX_KEY_STR_LEN];
    char                         list_str[MAX_LIST_VALUE_STR_LEN];
    sai_object_id_t              queue_oid;
    mlnx_port_config_t          *port;

    SX_LOG_ENTER();

    if (queue_id == NULL) {
        SX_LOG_ERR("Invalid NULL queue_id param\n");
        return SAI_STATUS_INVALID_PARAMETER;
    }

    status = check_attribs_metadata(attr_count, attr_list, queue_attribs, queue_vendor_attribs,
                                    SAI_COMMON_API_CREATE);
    if (SAI_ERR(status)) {
        return status;
    }

    sai_attr_list_to_str(attr_count, attr_list, queue_attribs, MAX_LIST_VALUE_STR_LEN, list_str);
    SX_LOG_NTC("Create queue, %s\n", list_str);

    /* Mandatory attributes */
    status = find_attrib_in_list(attr_count, attr_list, SAI_QUEUE_ATTR_TYPE, &type_attr, &type_idx);
    assert(SAI_STATUS_SUCCESS == status);

    status = find_attrib_in_list(attr_count, attr_list, SAI_QUEUE_ATTR_PORT, &port_attr, &port_idx);
    assert(SAI_STATUS_SUCCESS == status);

    status =
        find_attrib_in_list(attr_count, attr_list, SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE, &parent_attr, &parent_idx);
    assert(SAI_STATUS_SUCCESS == status);

    status = mlnx_object_to_type(port_attr->oid, SAI_OBJECT_TYPE_PORT, &port_id, NULL);
    if (SAI_ERR(status)) {
        goto out;
    }

    status = find_attrib_in_list(attr_count, attr_list, SAI_QUEUE_ATTR_INDEX, &index_attr, &index_idx);
    assert(SAI_STATUS_SUCCESS == status);

    status = mlnx_create_queue_object(port_id, index_attr->u8, &queue_oid);
    if (SAI_ERR(status)) {
        goto out;
    }

    sai_object_key_t object_key = { .key.object_id = queue_oid };

    sai_db_write_lock();

    status = mlnx_port_by_log_id(port_id, &port);
    if (SAI_ERR(status)) {
        sai_db_unlock();
        goto out;
    }

    status = mlnx_sched_hierarchy_reset(port);
    if (SAI_ERR(status)) {
        sai_db_unlock();
        goto out;
    }

    sai_db_unlock();

    status = mlnx_queue_parent_sched_node_set(&object_key, (const sai_attribute_value_t*)parent_attr, NULL);
    if (SAI_ERR(status)) {
        SX_LOG_ERR("Failed to set queue parent scheduler node %" PRIx64 "\n", parent_attr->oid);
        goto out;
    }

    /* Optional attributes */
    status = find_attrib_in_list(attr_count, attr_list, SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID,
                                 &sched_profile_attr, &sched_profile_idx);

    if (!SAI_ERR(status)) {
        status = mlnx_queue_config_set(&object_key, (const sai_attribute_value_t*)sched_profile_attr,
                                       (void*)SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID);

        if (SAI_ERR(status)) {
            goto out;
        }
    }

    status = find_attrib_in_list(attr_count, attr_list, SAI_QUEUE_ATTR_WRED_PROFILE_ID,
                                 &wred_profile_attr, &wred_profile_idx);

    if (!SAI_ERR(status)) {
        status = mlnx_queue_config_set(&object_key, (const sai_attribute_value_t*)wred_profile_attr,
                                       (void*)SAI_QUEUE_ATTR_WRED_PROFILE_ID);

        if (SAI_ERR(status)) {
            goto out;
        }
    }

    status = find_attrib_in_list(attr_count, attr_list, SAI_QUEUE_ATTR_BUFFER_PROFILE_ID,
                                 &buff_profile_attr, &buff_profile_idx);

    if (!SAI_ERR(status)) {
        status = mlnx_queue_config_set(&object_key, (const sai_attribute_value_t*)buff_profile_attr,
                                       (void*)SAI_QUEUE_ATTR_BUFFER_PROFILE_ID);

        if (SAI_ERR(status)) {
            goto out;
        }
    }

    queue_key_to_str(queue_oid, key_str);

    SX_LOG_NTC("Created %s\n", key_str);

    *queue_id = queue_oid;

out:
    SX_LOG_EXIT();
    return SAI_STATUS_SUCCESS;
}

/**
 * Routine Description:
 *    @brief Remove queue
 *
 * Arguments:
 *    @param[in] queue_id - queue id
 *
 * Return Values:
 *    @return SAI_STATUS_SUCCESS on success
 *            Failure status code on error
 */
sai_status_t mlnx_remove_queue(_In_ sai_object_id_t queue_id)
{
    sai_object_key_t      object_key = { .key.object_id = queue_id };
    char                  key_str[MAX_KEY_STR_LEN];
    sai_attribute_value_t parent_attr = { .oid = SAI_NULL_OBJECT_ID };
    sai_status_t          status;

    SX_LOG_ENTER();

    queue_key_to_str(queue_id, key_str);

    SX_LOG_NTC("Removing %s\n", key_str);

    status = mlnx_queue_parent_sched_node_set(&object_key, &parent_attr, NULL);
    if (SAI_ERR(status)) {
        SX_LOG_ERR("Failed to reset parent scheduler node for queue\n");
        goto out;
    }

    sai_db_write_lock();

    status = mlnx_scheduler_to_queue_apply(SAI_NULL_OBJECT_ID, queue_id);
    if (SAI_ERR(status)) {
        SX_LOG_ERR("Failed to reset scheduler profile for queue\n");
        goto out;
    }

    status = mlnx_buffer_apply(SAI_NULL_OBJECT_ID, queue_id);
    if (SAI_ERR(status)) {
        SX_LOG_ERR("Failed to reset buffer profile for queue\n");
        goto out;
    }

    status = mlnx_wred_apply(SAI_NULL_OBJECT_ID, queue_id);
    if (SAI_ERR(status)) {
        SX_LOG_ERR("Failed to reset wred profile for queue\n");
        goto out;
    }

out:
    sai_db_unlock();
    SX_LOG_EXIT();
    return status;
}

const sai_queue_api_t mlnx_queue_api = {
    mlnx_create_queue,
    mlnx_remove_queue,
    mlnx_set_queue_attribute,
    mlnx_get_queue_attribute,
    mlnx_get_queue_statistics,
    mlnx_clear_queue_stats
};
