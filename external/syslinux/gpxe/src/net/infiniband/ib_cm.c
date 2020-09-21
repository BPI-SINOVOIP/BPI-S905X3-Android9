/*
 * Copyright (C) 2009 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <byteswap.h>
#include <errno.h>
#include <assert.h>
#include <gpxe/infiniband.h>
#include <gpxe/ib_mi.h>
#include <gpxe/ib_pathrec.h>
#include <gpxe/ib_cm.h>

/**
 * @file
 *
 * Infiniband communication management
 *
 */

/** List of connections */
static LIST_HEAD ( ib_cm_conns );

/**
 * Send "ready to use" response
 *
 * @v ibdev		Infiniband device
 * @v mi		Management interface
 * @v conn		Connection
 * @v av		Address vector
 * @ret rc		Return status code
 */
static int ib_cm_send_rtu ( struct ib_device *ibdev,
			    struct ib_mad_interface *mi,
			    struct ib_connection *conn,
			    struct ib_address_vector *av ) {
	union ib_mad mad;
	struct ib_cm_ready_to_use *ready =
		&mad.cm.cm_data.ready_to_use;
	int rc;

	/* Construct "ready to use" response */
	memset ( &mad, 0, sizeof ( mad ) );
	mad.hdr.mgmt_class = IB_MGMT_CLASS_CM;
	mad.hdr.class_version = IB_CM_CLASS_VERSION;
	mad.hdr.method = IB_MGMT_METHOD_SEND;
	mad.hdr.attr_id = htons ( IB_CM_ATTR_READY_TO_USE );
	ready->local_id = htonl ( conn->local_id );
	ready->remote_id = htonl ( conn->remote_id );
	if ( ( rc = ib_mi_send ( ibdev, mi, &mad, av ) ) != 0 ){
		DBGC ( conn, "CM %p could not send RTU: %s\n",
		       conn, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Handle duplicate connection replies
 *
 * @v ibdev		Infiniband device
 * @v mi		Management interface
 * @v mad		Received MAD
 * @v av		Source address vector
 * @ret rc		Return status code
 *
 * If a "ready to use" MAD is lost, the peer may resend the connection
 * reply.  We have to respond to these with duplicate "ready to use"
 * MADs, otherwise the peer may time out and drop the connection.
 */
static void ib_cm_connect_rep ( struct ib_device *ibdev,
				struct ib_mad_interface *mi,
				union ib_mad *mad,
				struct ib_address_vector *av ) {
	struct ib_cm_connect_reply *connect_rep =
		&mad->cm.cm_data.connect_reply;
	struct ib_connection *conn;
	int rc;

	/* Identify connection */
	list_for_each_entry ( conn, &ib_cm_conns, list ) {
		if ( ntohl ( connect_rep->remote_id ) != conn->local_id )
			continue;
		/* Try to send "ready to use" reply */
		if ( ( rc = ib_cm_send_rtu ( ibdev, mi, conn, av ) ) != 0 ) {
			/* Ignore errors */
			return;
		}
		return;
	}

	DBG ( "CM unidentified connection %08x\n",
	      ntohl ( connect_rep->remote_id ) );
}

/** Communication management agents */
struct ib_mad_agent ib_cm_agent[] __ib_mad_agent = {
	{
		.mgmt_class = IB_MGMT_CLASS_CM,
		.class_version = IB_CM_CLASS_VERSION,
		.attr_id = htons ( IB_CM_ATTR_CONNECT_REPLY ),
		.handle = ib_cm_connect_rep,
	},
};

/**
 * Convert connection rejection reason to return status code
 *
 * @v reason		Rejection reason (in network byte order)
 * @ret rc		Return status code
 */
static int ib_cm_rejection_reason_to_rc ( uint16_t reason ) {
	switch ( reason ) {
	case htons ( IB_CM_REJECT_BAD_SERVICE_ID ) :
		return -ENODEV;
	case htons ( IB_CM_REJECT_STALE_CONN ) :
		return -EALREADY;
	case htons ( IB_CM_REJECT_CONSUMER ) :
		return -ENOTTY;
	default:
		return -EPERM;
	}
}

/**
 * Handle connection request transaction completion
 *
 * @v ibdev		Infiniband device
 * @v mi		Management interface
 * @v madx		Management transaction
 * @v rc		Status code
 * @v mad		Received MAD (or NULL on error)
 * @v av		Source address vector (or NULL on error)
 */
static void ib_cm_req_complete ( struct ib_device *ibdev,
				 struct ib_mad_interface *mi,
				 struct ib_mad_transaction *madx,
				 int rc, union ib_mad *mad,
				 struct ib_address_vector *av ) {
	struct ib_connection *conn = ib_madx_get_ownerdata ( madx );
	struct ib_queue_pair *qp = conn->qp;
	struct ib_cm_common *common = &mad->cm.cm_data.common;
	struct ib_cm_connect_reply *connect_rep =
		&mad->cm.cm_data.connect_reply;
	struct ib_cm_connect_reject *connect_rej =
		&mad->cm.cm_data.connect_reject;
	void *private_data = NULL;
	size_t private_data_len = 0;

	/* Report failures */
	if ( ( rc == 0 ) && ( mad->hdr.status != htons ( IB_MGMT_STATUS_OK ) ))
		rc = -EIO;
	if ( rc != 0 ) {
		DBGC ( conn, "CM %p connection request failed: %s\n",
		       conn, strerror ( rc ) );
		goto out;
	}

	/* Record remote communication ID */
	conn->remote_id = ntohl ( common->local_id );

	/* Handle response */
	switch ( mad->hdr.attr_id ) {

	case htons ( IB_CM_ATTR_CONNECT_REPLY ) :
		/* Extract fields */
		qp->av.qpn = ( ntohl ( connect_rep->local_qpn ) >> 8 );
		qp->send.psn = ( ntohl ( connect_rep->starting_psn ) >> 8 );
		private_data = &connect_rep->private_data;
		private_data_len = sizeof ( connect_rep->private_data );
		DBGC ( conn, "CM %p connected to QPN %lx PSN %x\n",
		       conn, qp->av.qpn, qp->send.psn );

		/* Modify queue pair */
		if ( ( rc = ib_modify_qp ( ibdev, qp ) ) != 0 ) {
			DBGC ( conn, "CM %p could not modify queue pair: %s\n",
			       conn, strerror ( rc ) );
			goto out;
		}

		/* Send "ready to use" reply */
		if ( ( rc = ib_cm_send_rtu ( ibdev, mi, conn, av ) ) != 0 ) {
			/* Treat as non-fatal */
			rc = 0;
		}
		break;

	case htons ( IB_CM_ATTR_CONNECT_REJECT ) :
		/* Extract fields */
		DBGC ( conn, "CM %p connection rejected (reason %d)\n",
		       conn, ntohs ( connect_rej->reason ) );
		/* Private data is valid only for a Consumer Reject */
		if ( connect_rej->reason == htons ( IB_CM_REJECT_CONSUMER ) ) {
			private_data = &connect_rej->private_data;
			private_data_len = sizeof (connect_rej->private_data);
		}
		rc = ib_cm_rejection_reason_to_rc ( connect_rej->reason );
		break;

	default:
		DBGC ( conn, "CM %p unexpected response (attribute %04x)\n",
		       conn, ntohs ( mad->hdr.attr_id ) );
		rc = -ENOTSUP;
		break;
	}

 out:
	/* Destroy the completed transaction */
	ib_destroy_madx ( ibdev, ibdev->gsi, madx );
	conn->madx = NULL;

	/* Hand off to the upper completion handler */
	conn->op->changed ( ibdev, qp, conn, rc, private_data,
			    private_data_len );
}

/** Connection request operations */
static struct ib_mad_transaction_operations ib_cm_req_op = {
	.complete = ib_cm_req_complete,
};

/**
 * Handle connection path transaction completion
 *
 * @v ibdev		Infiniband device
 * @v path		Path
 * @v rc		Status code
 * @v av		Address vector, or NULL on error
 */
static void ib_cm_path_complete ( struct ib_device *ibdev,
				  struct ib_path *path, int rc,
				  struct ib_address_vector *av ) {
	struct ib_connection *conn = ib_path_get_ownerdata ( path );
	struct ib_queue_pair *qp = conn->qp;
	union ib_mad mad;
	struct ib_cm_connect_request *connect_req =
		&mad.cm.cm_data.connect_request;
	size_t private_data_len;

	/* Report failures */
	if ( rc != 0 ) {
		DBGC ( conn, "CM %p path lookup failed: %s\n",
		       conn, strerror ( rc ) );
		conn->op->changed ( ibdev, qp, conn, rc, NULL, 0 );
		goto out;
	}

	/* Update queue pair peer path */
	memcpy ( &qp->av, av, sizeof ( qp->av ) );

	/* Construct connection request */
	memset ( &mad, 0, sizeof ( mad ) );
	mad.hdr.mgmt_class = IB_MGMT_CLASS_CM;
	mad.hdr.class_version = IB_CM_CLASS_VERSION;
	mad.hdr.method = IB_MGMT_METHOD_SEND;
	mad.hdr.attr_id = htons ( IB_CM_ATTR_CONNECT_REQUEST );
	connect_req->local_id = htonl ( conn->local_id );
	memcpy ( &connect_req->service_id, &conn->service_id,
		 sizeof ( connect_req->service_id ) );
	ib_get_hca_info ( ibdev, &connect_req->local_ca );
	connect_req->local_qpn__responder_resources =
		htonl ( ( qp->qpn << 8 ) | 1 );
	connect_req->local_eecn__initiator_depth = htonl ( ( 0 << 8 ) | 1 );
	connect_req->remote_eecn__remote_timeout__service_type__ee_flow_ctrl =
		htonl ( ( 0x14 << 3 ) | ( IB_CM_TRANSPORT_RC << 1 ) |
			( 0 << 0 ) );
	connect_req->starting_psn__local_timeout__retry_count =
		htonl ( ( qp->recv.psn << 8 ) | ( 0x14 << 3 ) |
			( 0x07 << 0 ) );
	connect_req->pkey = htons ( ibdev->pkey );
	connect_req->payload_mtu__rdc_exists__rnr_retry =
		( ( IB_MTU_2048 << 4 ) | ( 1 << 3 ) | ( 0x07 << 0 ) );
	connect_req->max_cm_retries__srq =
		( ( 0x0f << 4 ) | ( 0 << 3 ) );
	connect_req->primary.local_lid = htons ( ibdev->lid );
	connect_req->primary.remote_lid = htons ( conn->qp->av.lid );
	memcpy ( &connect_req->primary.local_gid, &ibdev->gid,
		 sizeof ( connect_req->primary.local_gid ) );
	memcpy ( &connect_req->primary.remote_gid, &conn->qp->av.gid,
		 sizeof ( connect_req->primary.remote_gid ) );
	connect_req->primary.flow_label__rate =
		htonl ( ( 0 << 12 ) | ( conn->qp->av.rate << 0 ) );
	connect_req->primary.hop_limit = 0;
	connect_req->primary.sl__subnet_local =
		( ( conn->qp->av.sl << 4 ) | ( 1 << 3 ) );
	connect_req->primary.local_ack_timeout = ( 0x13 << 3 );
	private_data_len = conn->private_data_len;
	if ( private_data_len > sizeof ( connect_req->private_data ) )
		private_data_len = sizeof ( connect_req->private_data );
	memcpy ( &connect_req->private_data, &conn->private_data,
		 private_data_len );

	/* Create connection request */
	av->qpn = IB_QPN_GSI;
	av->qkey = IB_QKEY_GSI;
	conn->madx = ib_create_madx ( ibdev, ibdev->gsi, &mad, av,
				      &ib_cm_req_op );
	if ( ! conn->madx ) {
		DBGC ( conn, "CM %p could not create connection request\n",
		       conn );
		conn->op->changed ( ibdev, qp, conn, rc, NULL, 0 );
		goto out;
	}
	ib_madx_set_ownerdata ( conn->madx, conn );

 out:
	/* Destroy the completed transaction */
	ib_destroy_path ( ibdev, path );
	conn->path = NULL;
}

/** Connection path operations */
static struct ib_path_operations ib_cm_path_op = {
	.complete = ib_cm_path_complete,
};

/**
 * Create connection to remote QP
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @v dgid		Target GID
 * @v service_id	Target service ID
 * @v private_data	Connection request private data
 * @v private_data_len	Length of connection request private data
 * @v op		Connection operations
 * @ret conn		Connection
 */
struct ib_connection *
ib_create_conn ( struct ib_device *ibdev, struct ib_queue_pair *qp,
		 struct ib_gid *dgid, struct ib_gid_half *service_id,
		 void *private_data, size_t private_data_len,
		 struct ib_connection_operations *op ) {
	struct ib_connection *conn;

	/* Allocate and initialise request */
	conn = zalloc ( sizeof ( *conn ) + private_data_len );
	if ( ! conn )
		goto err_alloc_conn;
	conn->ibdev = ibdev;
	conn->qp = qp;
	memset ( &qp->av, 0, sizeof ( qp->av ) );
	qp->av.gid_present = 1;
	memcpy ( &qp->av.gid, dgid, sizeof ( qp->av.gid ) );
	conn->local_id = random();
	memcpy ( &conn->service_id, service_id, sizeof ( conn->service_id ) );
	conn->op = op;
	conn->private_data_len = private_data_len;
	memcpy ( &conn->private_data, private_data, private_data_len );

	/* Create path */
	conn->path = ib_create_path ( ibdev, &qp->av, &ib_cm_path_op );
	if ( ! conn->path )
		goto err_create_path;
	ib_path_set_ownerdata ( conn->path, conn );

	/* Add to list of connections */
	list_add ( &conn->list, &ib_cm_conns );

	DBGC ( conn, "CM %p created for IBDEV %p QPN %lx\n",
	       conn, ibdev, qp->qpn );
	DBGC ( conn, "CM %p connecting to %08x:%08x:%08x:%08x %08x:%08x\n",
	       conn, ntohl ( dgid->u.dwords[0] ), ntohl ( dgid->u.dwords[1] ),
	       ntohl ( dgid->u.dwords[2] ), ntohl ( dgid->u.dwords[3] ),
	       ntohl ( service_id->u.dwords[0] ),
	       ntohl ( service_id->u.dwords[1] ) );

	return conn;

	ib_destroy_path ( ibdev, conn->path );
 err_create_path:
	free ( conn );
 err_alloc_conn:
	return NULL;
}

/**
 * Destroy connection to remote QP
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @v conn		Connection
 */
void ib_destroy_conn ( struct ib_device *ibdev,
		       struct ib_queue_pair *qp __unused,
		       struct ib_connection *conn ) {

	list_del ( &conn->list );
	if ( conn->madx )
		ib_destroy_madx ( ibdev, ibdev->gsi, conn->madx );
	if ( conn->path )
		ib_destroy_path ( ibdev, conn->path );
	free ( conn );
}
