/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018 6WIND S.A.
 * Copyright 2018 Mellanox Technologies, Ltd.
 */

#ifndef MLX5_GLUE_H_
#define MLX5_GLUE_H_

#include <stddef.h>
#include <stdint.h>

/* Verbs headers do not support -pedantic. */
#ifdef PEDANTIC
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <infiniband/verbs.h>
#ifdef PEDANTIC
#pragma GCC diagnostic error "-Wpedantic"
#endif

/* LIB_GLUE_VERSION must be updated every time this structure is modified. */
struct mlx5_glue {
	const char *version;
	void (*ack_async_event)(struct ibv_async_event *event);
	struct ibv_pd *(*alloc_pd)(struct ibv_context *context);
	int (*close_device)(struct ibv_context *context);
	int (*dealloc_pd)(struct ibv_pd *pd);
	int (*dereg_mr)(struct ibv_mr *mr);
	int (*destroy_cq)(struct ibv_cq *cq);
	int (*destroy_qp)(struct ibv_qp *qp);
	struct ibv_cq *(*exp_create_cq)
		(struct ibv_context *context, int cqe, void *cq_context,
		 struct ibv_comp_channel *channel, int comp_vector,
		 struct ibv_exp_cq_init_attr *attr);
	struct ibv_exp_flow *(*exp_create_flow)
		(struct ibv_qp *qp, struct ibv_exp_flow_attr *flow);
	struct ibv_qp *(*exp_create_qp)
		(struct ibv_context *context,
		 struct ibv_exp_qp_init_attr *qp_init_attr);
	struct ibv_exp_res_domain *(*exp_create_res_domain)
		(struct ibv_context *context,
		 struct ibv_exp_res_domain_init_attr *attr);
	struct ibv_exp_rwq_ind_table *(*exp_create_rwq_ind_table)
		(struct ibv_context *context,
		 struct ibv_exp_rwq_ind_table_init_attr *init_attr);
	struct ibv_exp_wq *(*exp_create_wq)
		(struct ibv_context *context,
		 struct ibv_exp_wq_init_attr *wq_init_attr);
	int (*exp_destroy_flow)(struct ibv_exp_flow *flow_id);
	int (*exp_destroy_res_domain)
		(struct ibv_context *context,
		 struct ibv_exp_res_domain *res_dom,
		 struct ibv_exp_destroy_res_domain_attr *attr);
	int (*exp_destroy_rwq_ind_table)
		(struct ibv_exp_rwq_ind_table *rwq_ind_table);
	int (*exp_destroy_wq)(struct ibv_exp_wq *wq);
	int (*exp_modify_qp)(struct ibv_qp *qp, struct ibv_exp_qp_attr *attr,
			     uint64_t exp_attr_mask);
	int (*exp_modify_wq)(struct ibv_exp_wq *wq,
			     struct ibv_exp_wq_attr *wq_attr);
	int (*exp_query_device)(struct ibv_context *context,
				struct ibv_exp_device_attr *attr);
	void *(*exp_query_intf)(struct ibv_context *context,
				struct ibv_exp_query_intf_params *params,
				enum ibv_exp_query_intf_status *status);
	int (*exp_release_intf)(struct ibv_context *context, void *intf,
				struct ibv_exp_release_intf_params *params);
	int (*fork_init)(void);
	void (*free_device_list)(struct ibv_device **list);
	int (*get_async_event)(struct ibv_context *context,
			       struct ibv_async_event *event);
	struct ibv_device **(*get_device_list)(int *num_devices);
	const char *(*get_device_name)(struct ibv_device *device);
	struct ibv_context *(*open_device)(struct ibv_device *device);
	const char *(*port_state_str)(enum ibv_port_state port_state);
	int (*query_device)(struct ibv_context *context,
			    struct ibv_device_attr *device_attr);
	int (*query_port)(struct ibv_context *context, uint8_t port_num,
			  struct ibv_port_attr *port_attr);
	struct ibv_mr *(*reg_mr)(struct ibv_pd *pd, void *addr, size_t length,
				 int access);
};

const struct mlx5_glue *mlx5_glue;

#endif /* MLX5_GLUE_H_ */
