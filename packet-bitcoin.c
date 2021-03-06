/* packet-bitcoin.c
 * Routines for bitcoin dissection
 * Copyright 2011, Christian Svensson <blue@cmd.nu>
 * Bitcoin address: 15Y2EN5mLnsTt3CZBfgpnZR5SeLwu7WEHz
 *
 * See https://en.bitcoin.it/wiki/Protocol_specification
 *
 * $Id: packet-bitcoin.c 54217 2013-12-18 13:15:13Z mmann $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib.h>

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/expert.h>

#include "packet-tcp.h"

#define BITCOIN_MAIN_MAGIC_NUMBER       0xD9B4BEF9
#define BITCOIN_TESTNET_MAGIC_NUMBER    0xDAB5BFFA
#define BITCOIN_TESTNET3_MAGIC_NUMBER   0x0709110B
#define LITECOIN_MAIN_MAGIC_NUMBER      0xDBB6C0FB
#define LITECOIN_TESTNET_MAGIC_NUMBER   0xDCB7C1FC
#define DOGECOIN_MAIN_MAGIC_NUMBER      0xC0C0C0C0
#define DOGECOIN_TESTNET_MAGIC_NUMBER   0xDCB7C1FC // dogecoin guys didn't change the magic number 

/*
 * Minimum bitcoin identification header.
 * - Magic - 4 bytes
 * - Command - 12 bytes
 * - Payload length - 4 bytes
 * - Checksum - 4 bytes
 */
#define BITCOIN_HEADER_LENGTH 4+12+4+4

void proto_register_bitcoin(void);
void proto_reg_handoff_bitcoin(void);

static int proto_bitcoin = -1;

static gint hf_bitcoin_magic = -1;
static gint hf_bitcoin_command = -1;
static gint hf_bitcoin_length = -1;
static gint hf_bitcoin_checksum = -1;



/* version message */
static gint hf_bitcoin_msg_version = -1;
static gint hf_msg_version_version = -1;
static gint hf_msg_version_services = -1;
static gint hf_msg_version_timestamp = -1;
static gint hf_msg_version_addr_me = -1;
static gint hf_msg_version_addr_you = -1;
static gint hf_msg_version_nonce = -1;
static gint hf_msg_version_user_agent_length8 = -1;
static gint hf_msg_version_user_agent_length16 = -1;
static gint hf_msg_version_user_agent_length32 = -1;
static gint hf_msg_version_user_agent_length64 = -1;
static gint hf_msg_version_user_agent = -1;
static gint hf_msg_version_start_height = -1;

/* addr message */
static gint hf_msg_addr_count8 = -1;
static gint hf_msg_addr_count16 = -1;
static gint hf_msg_addr_count32 = -1;
static gint hf_msg_addr_count64 = -1;
static gint hf_bitcoin_msg_addr = -1;
static gint hf_msg_addr_address = -1;
static gint hf_msg_addr_timestamp = -1;

/* inv message */
static gint hf_msg_inv_count8 = -1;
static gint hf_msg_inv_count16 = -1;
static gint hf_msg_inv_count32 = -1;
static gint hf_msg_inv_count64 = -1;
static gint hf_bitcoin_msg_inv = -1;
static gint hf_msg_inv_type = -1;
static gint hf_msg_inv_hash = -1;

/* getdata message */
static gint hf_msg_getdata_count8 = -1;
static gint hf_msg_getdata_count16 = -1;
static gint hf_msg_getdata_count32 = -1;
static gint hf_msg_getdata_count64 = -1;
static gint hf_bitcoin_msg_getdata = -1;
static gint hf_msg_getdata_type = -1;
static gint hf_msg_getdata_hash = -1;

/* notfound message */
static gint hf_msg_notfound_count8 = -1;
static gint hf_msg_notfound_count16 = -1;
static gint hf_msg_notfound_count32 = -1;
static gint hf_msg_notfound_count64 = -1;
static gint hf_bitcoin_msg_notfound = -1;
static gint hf_msg_notfound_type = -1;
static gint hf_msg_notfound_hash = -1;

/* getblocks message */
static gint hf_msg_getblocks_count8 = -1;
static gint hf_msg_getblocks_count16 = -1;
static gint hf_msg_getblocks_count32 = -1;
static gint hf_msg_getblocks_count64 = -1;
static gint hf_bitcoin_msg_getblocks = -1;
static gint hf_msg_getblocks_start = -1;
static gint hf_msg_getblocks_stop = -1;

/* getheaders message */
static gint hf_msg_getheaders_count8 = -1;
static gint hf_msg_getheaders_count16 = -1;
static gint hf_msg_getheaders_count32 = -1;
static gint hf_msg_getheaders_count64 = -1;
static gint hf_bitcoin_msg_getheaders = -1;
static gint hf_msg_getheaders_start = -1;
static gint hf_msg_getheaders_stop = -1;

/* tx message */
static gint hf_msg_tx_in_count8 = -1;
static gint hf_msg_tx_in_count16 = -1;
static gint hf_msg_tx_in_count32 = -1;
static gint hf_msg_tx_in_count64 = -1;
static gint hf_bitcoin_msg_tx = -1;
static gint hf_msg_tx_version = -1;
static gint hf_msg_tx_in_script8 = -1;
static gint hf_msg_tx_in_script16 = -1;
static gint hf_msg_tx_in_script32 = -1;
static gint hf_msg_tx_in_script64 = -1;
static gint hf_msg_tx_in = -1;
static gint hf_msg_tx_in_prev_output = -1;
static gint hf_msg_tx_in_prev_outp_hash = -1;
static gint hf_msg_tx_in_prev_outp_index = -1;
static gint hf_msg_tx_in_sig_script = -1;
static gint hf_msg_tx_in_seq = -1;
static gint hf_msg_tx_out_count8 = -1;
static gint hf_msg_tx_out_count16 = -1;
static gint hf_msg_tx_out_count32 = -1;
static gint hf_msg_tx_out_count64 = -1;
static gint hf_msg_tx_out = -1;
static gint hf_msg_tx_out_value = -1;
static gint hf_msg_tx_out_script8 = -1;
static gint hf_msg_tx_out_script16 = -1;
static gint hf_msg_tx_out_script32 = -1;
static gint hf_msg_tx_out_script64 = -1;
static gint hf_msg_tx_out_script = -1;
static gint hf_msg_tx_lock_time = -1;

/* block message */
static gint hf_msg_block_transactions8 = -1;
static gint hf_msg_block_transactions16 = -1;
static gint hf_msg_block_transactions32 = -1;
static gint hf_msg_block_transactions64 = -1;
static gint hf_bitcoin_msg_block = -1;
static gint hf_msg_block_version = -1;
static gint hf_msg_block_prev_block = -1;
static gint hf_msg_block_merkle_root = -1;
static gint hf_msg_block_time = -1;
static gint hf_msg_block_bits = -1;
static gint hf_msg_block_nonce = -1;

/* ping */
static gint hf_bitcoin_msg_ping = -1;
static gint hf_msg_ping_nonce = -1;

/* pong */
static gint hf_bitcoin_msg_pong = -1;
static gint hf_msg_pong_nonce = -1;

/* reject */
static gint hf_bitcoin_msg_reject = -1;
static gint hf_msg_reject_command = -1;
static gint hf_msg_reject_code = -1;
static gint hf_msg_reject_reason = -1;
static gint hf_msg_reject_hash = -1;

/* alert */
static gint hf_bitcoin_msg_alert = -1;
static gint hf_msg_alert_message = -1;
static gint hf_msg_alert_msg_length8 = -1;
static gint hf_msg_alert_msg_length16 = -1;
static gint hf_msg_alert_msg_length32 = -1;
static gint hf_msg_alert_msg_length64 = -1;
static gint hf_msg_alert_version = -1;
static gint hf_msg_alert_relayuntil = -1;
static gint hf_msg_alert_expiration = -1;
static gint hf_msg_alert_id = -1;
static gint hf_msg_alert_cancel = -1;
static gint hf_msg_alert_cancel_set_count8 = -1;
static gint hf_msg_alert_cancel_set_count16 = -1;
static gint hf_msg_alert_cancel_set_count32 = -1;
static gint hf_msg_alert_cancel_set_count64 = -1;
static gint hf_msg_alert_cancel_set_item = -1;
static gint hf_msg_alert_min_version = -1;
static gint hf_msg_alert_max_version = -1;
static gint hf_msg_alert_subver_set_count8 = -1;
static gint hf_msg_alert_subver_set_count16 = -1;
static gint hf_msg_alert_subver_set_count32 = -1;
static gint hf_msg_alert_subver_set_count64 = -1;
static gint hf_msg_alert_subver_set_str_length8 = -1;
static gint hf_msg_alert_subver_set_str_length16 = -1;
static gint hf_msg_alert_subver_set_str_length32 = -1;
static gint hf_msg_alert_subver_set_str_length64 = -1;
static gint hf_msg_alert_subver_set_string = -1;
static gint hf_msg_alert_priority = -1;
static gint hf_msg_alert_str_comment_length8= -1;
static gint hf_msg_alert_str_comment_length16= -1;
static gint hf_msg_alert_str_comment_length32= -1;
static gint hf_msg_alert_str_comment_length64= -1;
static gint hf_msg_alert_str_comment= -1;
static gint hf_msg_alert_str_status_bar_length8= -1;
static gint hf_msg_alert_str_status_bar_length16= -1;
static gint hf_msg_alert_str_status_bar_length32= -1;
static gint hf_msg_alert_str_status_bar_length64= -1;
static gint hf_msg_alert_str_status_bar= -1;
static gint hf_msg_alert_str_reserved_length8= -1;
static gint hf_msg_alert_str_reserved_length16= -1;
static gint hf_msg_alert_str_reserved_length32= -1;
static gint hf_msg_alert_str_reserved_length64= -1;
static gint hf_msg_alert_str_reserved= -1;
static gint hf_msg_alert_signature = -1;
static gint hf_msg_alert_signature_length8 = -1;
static gint hf_msg_alert_signature_length16 = -1;
static gint hf_msg_alert_signature_length32 = -1;
static gint hf_msg_alert_signature_length64 = -1;
static gint hf_msg_alert_signature_data = -1;

/* services */
static gint hf_services_network = -1;

/* address */
static gint hf_address_services = -1;
static gint hf_address_address = -1;
static gint hf_address_port = -1;


static gint ett_bitcoin = -1;
static gint ett_bitcoin_msg = -1;
static gint ett_services = -1;
static gint ett_address = -1;
static gint ett_ping = -1;
static gint ett_pong = -1;
static gint ett_reject = -1;
static gint ett_alert = -1;
static gint ett_alert_sig = -1;
static gint ett_alert_message = -1;
static gint ett_inv_list = -1;
static gint ett_getdata_list = -1;
static gint ett_notfound_list = -1;
static gint ett_getblocks_list = -1;
static gint ett_getheaders_list = -1;
static gint ett_tx_in_list = -1;
static gint ett_tx_in_outp = -1;
static gint ett_tx_out_list = -1;

static dissector_handle_t bitcoin_handle;
static gboolean bitcoin_desegment  = TRUE;

static const value_string magic_types[] =
{
  { 0xF9BEB4D9, "MAIN" },
  { 0xFABFBFDA, "REGTESTNET" },
  { 0x0B110907, "TESTNET3" },
  { 0xFBC0B6DB, "LITECOIN" },
  { 0xFCC1B7DC, "LITETESTNET" },
  { 0xC0C0C0C0, "DOGECOIN" },
  { 0, NULL }
};
static const value_string inv_types[] =
{
  { 0, "ERROR" },
  { 1, "MSG_TX" },
  { 2, "MSG_BLOCK" },
  { 0, NULL }
};

static const value_string msg_reject_codes[] =
{
  { 0x01, "REJECT_MALFORMED" },
  { 0x10, "REJECT_INVALID" },
  { 0x11, "REJECT_OBSOLETE" },
  { 0x12, "REJECT_DUPLICATE" },
  { 0x40, "REJECT_NONSTANDARD" },
  { 0x41, "REJECT_DUST" },
  { 0x42, "REJECT_INSUFFICIENTFEE" },
  { 0x43, "REJECT_CHECKPOINT" },
  { 0, NULL }
};


static guint
get_bitcoin_pdu_length(packet_info *pinfo _U_, tvbuff_t *tvb, int offset)
{
  guint32 length;
  length = BITCOIN_HEADER_LENGTH;

  /* add payload length */
  length += tvb_get_letohl(tvb, offset+16);

  return length;
}

/**
 * Create a services sub-tree for bit-by-bit display
 */
static proto_tree *
create_services_tree(tvbuff_t *tvb, proto_item *ti, guint32 offset)
{
  proto_tree *tree;
  guint64 services;

  tree = proto_item_add_subtree(ti, ett_services);

  /* start of services */
  /* NOTE:
   *  - 2011-06-05
   *    Currently the boolean tree only supports a maximum of
   *    32 bits - so we split services in two
   */
  services = tvb_get_letoh64(tvb, offset);

  /* service = NODE_NETWORK */
  proto_tree_add_boolean(tree, hf_services_network, tvb, offset, 4, (guint32)services);

  /* end of services */

  return tree;
}

/**
 * Create a sub-tree and fill it with a net_addr structure
 */
static proto_tree *
create_address_tree(tvbuff_t *tvb, proto_item *ti, guint32 offset)
{
  proto_tree *tree;

  tree = proto_item_add_subtree(ti, ett_address);

  /* services */
  ti = proto_tree_add_item(tree, hf_address_services, tvb, offset, 8, ENC_LITTLE_ENDIAN);
  create_services_tree(tvb, ti, offset);
  offset += 8;

  /* IPv6 address */
  proto_tree_add_item(tree, hf_address_address, tvb, offset, 16, ENC_NA);
  offset += 16;

  /* port */
  proto_tree_add_item(tree, hf_address_port, tvb, offset, 2, ENC_BIG_ENDIAN);

  return tree;
}

/**
 * Extract a variable length integer from a tvbuff
 */
static void
get_varint(tvbuff_t *tvb, const gint offset, gint *length, guint64 *ret)
{
  guint value;

  /* Note: just throw an exception if not enough  bytes are available in the tvbuff */

  /* calculate variable length */
  value = tvb_get_guint8(tvb, offset);
  if (value < 0xfd)
  {
    *length = 1;
    *ret = value;
    return;
  }

  if (value == 0xfd)
  {
    *length = 3;
    *ret = tvb_get_letohs(tvb, offset+1);
    return;
  }
  if (value == 0xfe)
  {
    *length = 5;
    *ret = tvb_get_letohl(tvb, offset+1);
    return;
  }

  *length = 9;
  *ret = tvb_get_letoh64(tvb, offset+1);
  return;

}

static void add_varint_item(proto_tree *tree, tvbuff_t *tvb, const gint offset, gint length,
                            gint hf8, gint hf16, gint hf32, gint hf64)
{
  switch (length)
  {
  case 1:
    proto_tree_add_item(tree, hf8,  tvb, offset, 1, ENC_LITTLE_ENDIAN);
    break;
  case 3:
    proto_tree_add_item(tree, hf16, tvb, offset+1, 2, ENC_LITTLE_ENDIAN);
    break;
  case 5:
    proto_tree_add_item(tree, hf32, tvb, offset+1, 4, ENC_LITTLE_ENDIAN);
    break;
  case 9:
    proto_tree_add_item(tree, hf64, tvb, offset+1, 8, ENC_LITTLE_ENDIAN);
    break;
  }
}

/* Note: A number of the following message handlers include code of the form:
 *          ...
 *          guint64     count;
 *          ...
 *          for (; count > 0; count--)
 *          {
 *            proto_tree_add_item9...);
 *            offset += ...;
 *            proto_tree_add_item9...);
 *            offset += ...;
 *            ...
 *          }
 *          ...
 *
 * Issue if 'count' is a very large number:
 *    If 'tree' is NULL, then the result will be effectively (but not really)
 *    an infinite loop. This is true because if 'tree' is NULL then
 *    proto_tree_add_item(tree, ...) is effectively a no-op and will not throw
 *    an exception.
 *    So: the loop should be executed only when 'tree' is defined so that the
 *        proto_ calls will throw an exception when the tvb is used up;
 *        This should only take a few-hundred loops at most.
 *           https://bugs.wireshark.org/bugzilla/show_bug.cgi?id=8312
 */

/**
 * Handler for version messages
 */
static void
dissect_bitcoin_msg_version(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  gint        varint_length;
  guint64     user_agent_length;
  guint32     offset = 0;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_version, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_bitcoin_msg);

  proto_tree_add_item(tree, hf_msg_version_version, tvb, offset, 4, ENC_LITTLE_ENDIAN);
  offset += 4;

  ti = proto_tree_add_item(tree, hf_msg_version_services, tvb, offset, 8, ENC_LITTLE_ENDIAN);
  create_services_tree(tvb, ti, offset);
  offset += 8;

  proto_tree_add_item(tree, hf_msg_version_timestamp, tvb, offset, 8, ENC_TIME_TIMESPEC|ENC_LITTLE_ENDIAN);
  offset += 8;

  ti = proto_tree_add_item(tree, hf_msg_version_addr_me, tvb, offset, 26, ENC_NA);
  create_address_tree(tvb, ti, offset);
  offset += 26;

  ti = proto_tree_add_item(tree, hf_msg_version_addr_you, tvb, offset, 26, ENC_NA);
  create_address_tree(tvb, ti, offset);
  offset += 26;

  proto_tree_add_item(tree, hf_msg_version_nonce, tvb, offset, 8, ENC_LITTLE_ENDIAN);
  offset += 8;

  /* find var_str user_agent */

  get_varint(tvb, offset, &varint_length, &user_agent_length);
  add_varint_item(tree, tvb, offset, varint_length, hf_msg_version_user_agent_length8, hf_msg_version_user_agent_length16,
                  hf_msg_version_user_agent_length32, hf_msg_version_user_agent_length64);
  offset += varint_length;

  proto_tree_add_item(tree, hf_msg_version_user_agent, tvb, offset, user_agent_length, ENC_ASCII|ENC_NA);
  offset += user_agent_length;

  proto_tree_add_item(tree, hf_msg_version_start_height, tvb, offset, 4, ENC_LITTLE_ENDIAN);
}

/*
 * Handler for address messages
 */
static void
dissect_bitcoin_msg_addr(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  gint        length;
  guint64     count;
  guint32     offset = 0;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_addr, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_bitcoin_msg);

  get_varint(tvb, offset, &length, &count);
  add_varint_item(tree, tvb, offset, length, hf_msg_addr_count8, hf_msg_addr_count16,
                  hf_msg_addr_count32, hf_msg_addr_count64);
  offset += length;

  for (; count > 0; count--)
  {
    proto_tree *subtree;

    ti = proto_tree_add_item(tree, hf_msg_addr_address, tvb, offset, 30, ENC_NA);
    subtree = create_address_tree(tvb, ti, offset+4);

    proto_tree_add_item(subtree, hf_msg_addr_timestamp, tvb, offset, 4, ENC_TIME_TIMESPEC|ENC_LITTLE_ENDIAN);
    offset += 26;
    offset += 4;
  }
}

/**
 * Handler for inventory messages
 */
static void
dissect_bitcoin_msg_inv(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  gint        length;
  guint64     count;
  guint32     offset = 0;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_inv, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_bitcoin_msg);

  get_varint(tvb, offset, &length, &count);
  add_varint_item(tree, tvb, offset, length, hf_msg_inv_count8, hf_msg_inv_count16,
                  hf_msg_inv_count32, hf_msg_inv_count64);

  offset += length;

  for (; count > 0; count--)
  {
    proto_tree *subtree;

    ti = proto_tree_add_text(tree, tvb, offset, 36, "Inventory vector");
    subtree = proto_item_add_subtree(ti, ett_inv_list);

    proto_tree_add_item(subtree, hf_msg_inv_type, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;

    proto_tree_add_item(subtree, hf_msg_inv_hash, tvb, offset, 32, ENC_NA);
    offset += 32;
  }
}

/**
 * Handler for getdata messages
 */
static void
dissect_bitcoin_msg_getdata(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  gint        length;
  guint64     count;
  guint32     offset = 0;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_getdata, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_bitcoin_msg);

  get_varint(tvb, offset, &length, &count);
  add_varint_item(tree, tvb, offset, length, hf_msg_getdata_count8, hf_msg_getdata_count16,
                  hf_msg_getdata_count32, hf_msg_getdata_count64);

  offset += length;

  for (; count > 0; count--)
  {
    proto_tree *subtree;

    ti = proto_tree_add_text(tree, tvb, offset, 36, "Inventory vector");
    subtree = proto_item_add_subtree(ti, ett_getdata_list);

    proto_tree_add_item(subtree, hf_msg_getdata_type, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;

    proto_tree_add_item(subtree, hf_msg_getdata_hash, tvb, offset, 32, ENC_NA);
    offset += 32;
  }
}

/**
 * Handler for notfound messages
 */
static void
dissect_bitcoin_msg_notfound(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  gint        length;
  guint64     count;
  guint32     offset = 0;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_notfound, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_bitcoin_msg);

  get_varint(tvb, offset, &length, &count);
  add_varint_item(tree, tvb, offset, length, hf_msg_notfound_count8, hf_msg_notfound_count16,
                  hf_msg_notfound_count32, hf_msg_notfound_count64);

  offset += length;

  for (; count > 0; count--)
  {
    proto_tree *subtree;

    ti = proto_tree_add_text(tree, tvb, offset, 36, "Inventory vector");
    subtree = proto_item_add_subtree(ti, ett_notfound_list);

    proto_tree_add_item(subtree, hf_msg_notfound_type, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;

    proto_tree_add_item(subtree, hf_msg_notfound_hash, tvb, offset, 32, ENC_NA);
    offset += 32;
  }
}

/**
 * Handler for getblocks messages
 */
static void
dissect_bitcoin_msg_getblocks(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  gint        length;
  guint64     count;
  guint32     offset = 0;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_getblocks, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_bitcoin_msg);

  /* why the protcol version is sent here nobody knows */
  proto_tree_add_item(tree, hf_msg_version_version, tvb, offset, 4, ENC_LITTLE_ENDIAN);
  offset += 4;

  get_varint(tvb, offset, &length, &count);
  add_varint_item(tree, tvb, offset, length, hf_msg_getblocks_count8, hf_msg_getblocks_count16,
                  hf_msg_getblocks_count32, hf_msg_getblocks_count64);

  offset += length;

  for (; count > 0; count--)
  {
    proto_tree_add_item(tree, hf_msg_getblocks_start, tvb, offset, 32, ENC_NA);
    offset += 32;
  }

  proto_tree_add_item(tree, hf_msg_getblocks_stop, tvb, offset, 32, ENC_NA);
}

/**
 * Handler for getheaders messages
 * UNTESTED
 */
static void
dissect_bitcoin_msg_getheaders(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  gint        length;
  guint64     count;
  guint32     offset = 0;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_getheaders, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_bitcoin_msg);

  get_varint(tvb, offset, &length, &count);
  add_varint_item(tree, tvb, offset, length, hf_msg_getheaders_count8, hf_msg_getheaders_count16,
                  hf_msg_getheaders_count32, hf_msg_getheaders_count64);

  offset += length;

  for (; count > 0; count--)
  {
    proto_tree_add_item(tree, hf_msg_getheaders_start, tvb, offset, 32, ENC_NA);
    offset += 32;
  }

  proto_tree_add_item(tree, hf_msg_getheaders_stop, tvb, offset, 32, ENC_NA);
  return;
}

/**
 * Handler for tx message body
 */
static guint32
dissect_bitcoin_msg_tx_common(tvbuff_t *tvb, guint32 offset, packet_info *pinfo _U_, proto_tree *tree, guint msgnum)
{
  proto_item *rti;
  gint        count_length;
  guint64     in_count;
  guint64     out_count;

  DISSECTOR_ASSERT(tree != NULL);

  if (msgnum == 0) {
    rti  = proto_tree_add_item(tree, hf_bitcoin_msg_tx, tvb, offset, -1, ENC_NA);
  } else {
    rti  = proto_tree_add_none_format(tree, hf_bitcoin_msg_tx, tvb, offset, -1, "Tx message [ %4d ]", msgnum);
  }
  tree = proto_item_add_subtree(rti, ett_bitcoin_msg);

  proto_tree_add_item(tree, hf_msg_tx_version, tvb, offset, 4, ENC_LITTLE_ENDIAN);
  offset += 4;

  /* TxIn[] */
  get_varint(tvb, offset, &count_length, &in_count);
  add_varint_item(tree, tvb, offset, count_length, hf_msg_tx_in_count8, hf_msg_tx_in_count16,
                  hf_msg_tx_in_count32, hf_msg_tx_in_count64);

  offset += count_length;

  /* TxIn
   *   [36]  previous_output    outpoint
   *   [1+]  script length      var_int
   *   [ ?]  signature script   uchar[]
   *   [ 4]  sequence           uint32_t
   *
   * outpoint (aka previous output)
   *   [32]  hash               char[32
   *   [ 4]  index              uint32_t
   *
   */
  for (; in_count > 0; in_count--)
  {
    proto_tree *subtree;
    proto_tree *prevtree;
    proto_item *ti;
    proto_item *pti;
    guint64     script_length;

    get_varint(tvb, offset+36, &count_length, &script_length);

    /* A funny script_length won't cause an exception since the field type is FT_NONE */
    ti = proto_tree_add_item(tree, hf_msg_tx_in, tvb, offset,
        36 + count_length + (guint)script_length + 4, ENC_NA);
    subtree = proto_item_add_subtree(ti, ett_tx_in_list);

    /* previous output */
    pti = proto_tree_add_item(subtree, hf_msg_tx_in_prev_output, tvb, offset, 36, ENC_NA);
    prevtree = proto_item_add_subtree(pti, ett_tx_in_outp);

    proto_tree_add_item(prevtree, hf_msg_tx_in_prev_outp_hash, tvb, offset, 32, ENC_NA);
    offset += 32;

    proto_tree_add_item(prevtree, hf_msg_tx_in_prev_outp_index, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    /* end previous output */

    add_varint_item(subtree, tvb, offset, count_length, hf_msg_tx_in_script8, hf_msg_tx_in_script16,
                    hf_msg_tx_in_script32, hf_msg_tx_in_script64);

    offset += count_length;

    if ((offset + script_length) > G_MAXINT)
      THROW(ReportedBoundsError);  /* special check since script_length is guint64 */

    proto_tree_add_item(subtree, hf_msg_tx_in_sig_script, tvb, offset, (guint)script_length, ENC_NA);
    offset += (guint)script_length;

    proto_tree_add_item(subtree, hf_msg_tx_in_seq, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
  }

  /* TxOut[] */
  get_varint(tvb, offset, &count_length, &out_count);
  add_varint_item(tree, tvb, offset, count_length, hf_msg_tx_out_count8, hf_msg_tx_out_count16,
                  hf_msg_tx_out_count32, hf_msg_tx_out_count64);

  offset += count_length;

  /*  TxOut
   *    [ 8] value
   *    [1+] script length [var_int]
   *    [ ?] script
   */
  for (; out_count > 0; out_count--)
  {
    proto_item *ti;
    proto_tree *subtree;
    guint64     script_length;

    get_varint(tvb, offset+8, &count_length, &script_length);

    /* A funny script_length won't cause an exception since the field type is FT_NONE */
    ti = proto_tree_add_item(tree, hf_msg_tx_out, tvb, offset,
                             8 + count_length + (guint)script_length , ENC_NA);
    subtree = proto_item_add_subtree(ti, ett_tx_out_list);

    proto_tree_add_item(subtree, hf_msg_tx_out_value, tvb, offset, 8, ENC_LITTLE_ENDIAN);
    offset += 8;

    add_varint_item(subtree, tvb, offset, count_length, hf_msg_tx_out_script8, hf_msg_tx_out_script16,
                    hf_msg_tx_out_script32, hf_msg_tx_out_script64);

    offset += count_length;

    if ((offset + script_length) > G_MAXINT)
      THROW(ReportedBoundsError);  /* special check since script_length is guint64 */

    proto_tree_add_item(subtree, hf_msg_tx_out_script, tvb, offset, (guint)script_length, ENC_NA);
    offset += (guint)script_length;
  }

  proto_tree_add_item(tree, hf_msg_tx_lock_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
  offset += 4;

  /* needed for block nesting */
  proto_item_set_len(rti, offset);

  return offset;
}

/**
 * Handler for tx message
 */
static void
dissect_bitcoin_msg_tx(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  if (!tree)
    return;

  dissect_bitcoin_msg_tx_common(tvb, 0, pinfo, tree, 0);
}


/**
 * Handler for block messages
 */

static void
dissect_bitcoin_msg_block(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_item *ti;
  gint        length;
  guint64     count;
  guint       msgnum;
  guint32     offset = 0;

  if (!tree)
    return;

  /*  Block
   *    [ 4] version         uint32_t
   *    [32] prev_block      char[32]
   *    [32] merkle_root     char[32]
   *    [ 4] timestamp       uint32_t  A unix timestamp ... (Currently limited to dates before the year 2106!)
   *    [ 4] bits            uint32_t
   *    [ 4] nonce           uint32_t
   *    [ ?] txn_count       var_int
   *    [ ?] txns            tx[]      Block transactions, in format of "tx" command
   */

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_block, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_bitcoin_msg);

  proto_tree_add_item(tree, hf_msg_block_version,     tvb, offset,  4, ENC_LITTLE_ENDIAN);
  offset += 4;

  proto_tree_add_item(tree, hf_msg_block_prev_block,  tvb, offset, 32, ENC_NA);
  offset += 32;

  proto_tree_add_item(tree, hf_msg_block_merkle_root, tvb, offset, 32, ENC_NA);
  offset += 32;

  proto_tree_add_item(tree, hf_msg_block_time,        tvb, offset,  4, ENC_TIME_TIMESPEC|ENC_LITTLE_ENDIAN);
  offset += 4;

  proto_tree_add_item(tree, hf_msg_block_bits,        tvb, offset,  4, ENC_LITTLE_ENDIAN);
  offset += 4;

  proto_tree_add_item(tree, hf_msg_block_nonce,       tvb, offset,  4, ENC_LITTLE_ENDIAN);
  offset += 4;

  get_varint(tvb, offset, &length, &count);
  add_varint_item(tree, tvb, offset, length, hf_msg_block_transactions8, hf_msg_block_transactions16,
                  hf_msg_block_transactions32, hf_msg_block_transactions64);

  offset += length;

  msgnum = 0;
  for (; count > 0; count--)
  {
    msgnum += 1;
    offset = dissect_bitcoin_msg_tx_common(tvb, offset, pinfo, tree, msgnum);
  }
}
/*
 * Handler for ping messages
 */
static void
dissect_bitcoin_msg_ping(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  guint32     offset = 0;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_ping, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_ping);

  proto_tree_add_item(tree, hf_msg_ping_nonce, tvb, offset, 8, ENC_LITTLE_ENDIAN);
  offset += 8;

}

/*
 * Handler for pong messages
 */
static void
dissect_bitcoin_msg_pong(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  guint32     offset = 0;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_pong, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_pong);

  proto_tree_add_item(tree, hf_msg_pong_nonce, tvb, offset, 8, ENC_LITTLE_ENDIAN);
  offset += 8;

}

/*
 * Handler for reject messages
 */
static void
dissect_bitcoin_msg_reject(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  guint32     offset = 0;
  gint        varint_length;
  guint64     str_length;
  //guint8      error_code;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_reject, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_reject);

  /* find var_str user_agent */

  get_varint(tvb, offset, &varint_length, &str_length);
  offset += varint_length;

  proto_tree_add_item(tree, hf_msg_reject_command, tvb, offset, str_length, ENC_ASCII|ENC_NA);
  offset += str_length;

  //error_code = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tree, hf_msg_reject_code, tvb, offset, 1, ENC_LITTLE_ENDIAN);
  offset ++;

  get_varint(tvb, offset, &varint_length, &str_length);
  offset += varint_length;

  proto_tree_add_item(tree, hf_msg_reject_reason, tvb, offset, str_length, ENC_ASCII|ENC_NA);
  offset += str_length;

  proto_tree_add_item(tree, hf_msg_reject_hash, tvb, offset, 32, ENC_NA);
  offset += 32;
}

/*
 * Handler for alert messages
 */
static void
dissect_bitcoin_msg_alert(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree)
{
  proto_item *ti;
  guint32     offset = 0;
  gint        varint_length;
  guint64     msg_length,sig_length,set_length,str_length;
  proto_tree  *subtree;

  if (!tree)
    return;

  ti   = proto_tree_add_item(tree, hf_bitcoin_msg_alert, tvb, offset, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_alert);

  /* message portion*/
  get_varint(tvb, offset, &varint_length, &msg_length);

  ti   = proto_tree_add_item(tree, hf_msg_alert_message, tvb, offset, varint_length+msg_length, ENC_NA);
  subtree = proto_item_add_subtree(ti, ett_alert_message);

  add_varint_item(subtree, tvb, offset, varint_length, hf_msg_alert_msg_length8, hf_msg_alert_msg_length16,
                  hf_msg_alert_msg_length32, hf_msg_alert_msg_length64);
  offset += varint_length;

  // version 
  proto_tree_add_item(subtree, hf_msg_alert_version,  tvb, offset,  4, ENC_LITTLE_ENDIAN);
  offset += 4;

  // times 
  proto_tree_add_item(subtree, hf_msg_alert_relayuntil, tvb, offset, 8, ENC_TIME_TIMESPEC|ENC_LITTLE_ENDIAN);
  offset += 8;
  proto_tree_add_item(subtree, hf_msg_alert_expiration, tvb, offset, 8, ENC_TIME_TIMESPEC|ENC_LITTLE_ENDIAN);
  offset += 8;

  // id & cancel 
  proto_tree_add_item(subtree, hf_msg_alert_id,  tvb, offset,  4, ENC_LITTLE_ENDIAN);
  offset += 4;
  proto_tree_add_item(subtree, hf_msg_alert_cancel,  tvb, offset,  4, ENC_LITTLE_ENDIAN);
  offset += 4;

  // cancel set 
  get_varint(tvb, offset, &varint_length, &set_length);
  add_varint_item(subtree, tvb, offset, varint_length, hf_msg_alert_cancel_set_count8, hf_msg_alert_cancel_set_count16,
                  hf_msg_alert_cancel_set_count32, hf_msg_alert_cancel_set_count64);
  offset += varint_length;

  for (; set_length > 0; set_length--)
  {
    // **** should put this in a sub list
    //subtree = proto_item_add_subtree(ti, ett_inv_list);

    proto_tree_add_item(subtree, hf_msg_alert_cancel_set_item, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
  }

  proto_tree_add_item(subtree, hf_msg_alert_min_version,  tvb, offset,  4, ENC_LITTLE_ENDIAN);
  offset += 4;
  proto_tree_add_item(subtree, hf_msg_alert_max_version,  tvb, offset,  4, ENC_LITTLE_ENDIAN);
  offset += 4;


  // subver set 
  get_varint(tvb, offset, &varint_length, &set_length);
  add_varint_item(subtree, tvb, offset, varint_length, hf_msg_alert_subver_set_count8, hf_msg_alert_subver_set_count16,
                  hf_msg_alert_subver_set_count32, hf_msg_alert_subver_set_count64);
  offset += varint_length;

  for (; set_length > 0; set_length--)
  {
    // **** should put this in a sub list
    //subtree = proto_item_add_subtree(ti, ett_inv_list);

    guint64 subver_length;

    get_varint(tvb, offset, &varint_length, &subver_length);
    add_varint_item(subtree, tvb, offset, varint_length,hf_msg_alert_subver_set_str_length8, 
                          hf_msg_alert_subver_set_str_length16, 
                          hf_msg_alert_subver_set_str_length32,hf_msg_alert_subver_set_str_length64); 
    offset += varint_length;

    proto_tree_add_item(subtree,hf_msg_alert_subver_set_string, tvb, offset, subver_length, ENC_ASCII|ENC_NA);
    offset += subver_length;

  }

  // priority

  proto_tree_add_item(subtree, hf_msg_alert_priority,  tvb, offset,  4, ENC_LITTLE_ENDIAN);
  offset += 4;

  // string messages -- comment
    get_varint(tvb, offset, &varint_length, &str_length);
    add_varint_item(subtree, tvb, offset, varint_length,hf_msg_alert_str_comment_length8, 
                          hf_msg_alert_str_comment_length16, 
                          hf_msg_alert_str_comment_length32,hf_msg_alert_str_comment_length64); 
    offset += varint_length;

    proto_tree_add_item(subtree,hf_msg_alert_str_comment, tvb, offset, str_length, ENC_ASCII|ENC_NA);
    offset += str_length;

  // string messages -- status bar
    get_varint(tvb, offset, &varint_length, &str_length);
    add_varint_item(subtree, tvb, offset, varint_length,hf_msg_alert_str_status_bar_length8, 
                          hf_msg_alert_str_status_bar_length16, 
                          hf_msg_alert_str_status_bar_length32,hf_msg_alert_str_status_bar_length64); 
    offset += varint_length;

    proto_tree_add_item(subtree,hf_msg_alert_str_status_bar, tvb, offset, str_length, ENC_ASCII|ENC_NA);
    offset += str_length;

  // string messages -- reserved
    get_varint(tvb, offset, &varint_length, &str_length);
    add_varint_item(subtree, tvb, offset, varint_length,hf_msg_alert_str_reserved_length8, 
                          hf_msg_alert_str_reserved_length16, 
                          hf_msg_alert_str_reserved_length32,hf_msg_alert_str_reserved_length64); 
    offset += varint_length;

    proto_tree_add_item(subtree,hf_msg_alert_str_reserved, tvb, offset, str_length, ENC_ASCII|ENC_NA);
    offset += str_length;

  /* signature portion*/
  ti   = proto_tree_add_item(tree, hf_msg_alert_signature, tvb, offset, -1, ENC_NA);
  subtree = proto_item_add_subtree(ti, ett_alert_sig);
  get_varint(tvb, offset, &varint_length, &sig_length);
  offset += varint_length;
  add_varint_item(subtree, tvb, offset, varint_length, hf_msg_alert_signature_length8, hf_msg_alert_signature_length16,
                  hf_msg_alert_signature_length32, hf_msg_alert_signature_length64);
  proto_tree_add_item(subtree, hf_msg_alert_signature_data, tvb, offset, sig_length, ENC_NA);
  offset += sig_length;
}

/**
 * Handler for unimplemented or payload-less messages
 */
static void
dissect_bitcoin_msg_empty(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_)
{
  return;
}

typedef void (*msg_dissector_func_t)(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

typedef struct msg_dissector
{
  const gchar *command;
  msg_dissector_func_t function;
} msg_dissector_t;

static msg_dissector_t msg_dissectors[] =
{
  {"version",     dissect_bitcoin_msg_version},
  {"addr",        dissect_bitcoin_msg_addr},
  {"inv",         dissect_bitcoin_msg_inv},
  {"getdata",     dissect_bitcoin_msg_getdata},
  {"notfound",    dissect_bitcoin_msg_notfound},
  {"getblocks",   dissect_bitcoin_msg_getblocks},
  {"getheaders",  dissect_bitcoin_msg_getheaders},
  {"tx",          dissect_bitcoin_msg_tx},
  {"block",       dissect_bitcoin_msg_block},
  {"ping",        dissect_bitcoin_msg_ping},
  {"pong",        dissect_bitcoin_msg_pong},
  {"reject",      dissect_bitcoin_msg_reject},
  {"alert",       dissect_bitcoin_msg_alert},

  /* messages with no payload */
  {"verack",      dissect_bitcoin_msg_empty},
  {"getaddr",     dissect_bitcoin_msg_empty},
  {"mempool",     dissect_bitcoin_msg_empty},

  /* messages not implemented */
  {"headers",     dissect_bitcoin_msg_empty},
  {"checkorder",  dissect_bitcoin_msg_empty},
  {"submitorder", dissect_bitcoin_msg_empty},
  {"reply",       dissect_bitcoin_msg_empty},
  {"filterload",  dissect_bitcoin_msg_empty},
  {"filteradd",   dissect_bitcoin_msg_empty},
  {"filterclear", dissect_bitcoin_msg_empty},
  {"merkleblock", dissect_bitcoin_msg_empty},
};

//////////////////////////////////
////// dissect_bitcoin_tcp_pdu
////// Main disector entry point after multiple pdus resolved
////// 
////// dissect packet
//////////////////////////////////
static void dissect_bitcoin_tcp_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  proto_item *ti;
  guint32     i;
  guint32     offset = 0;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "Bitcoin");
  col_clear(pinfo->cinfo, COL_INFO);

  ti   = proto_tree_add_item(tree, proto_bitcoin, tvb, 0, -1, ENC_NA);
  tree = proto_item_add_subtree(ti, ett_bitcoin);

  /* add basic protocol data */
  proto_tree_add_item(tree, hf_bitcoin_magic,   tvb,  0,  4, ENC_BIG_ENDIAN);
  proto_tree_add_item(tree, hf_bitcoin_command, tvb,  4, 12, ENC_ASCII|ENC_NA);
  proto_tree_add_item(tree, hf_bitcoin_length,  tvb, 16,  4, ENC_LITTLE_ENDIAN);
  proto_tree_add_item(tree, hf_bitcoin_checksum, tvb, 20, 4, ENC_BIG_ENDIAN);
  offset = BITCOIN_HEADER_LENGTH;

  /* TODO: verify checksum? */

  /* handle command specific message part */
  for (i = 0; i < array_length(msg_dissectors); i++)
  {
    if (tvb_memeql(tvb, 4, msg_dissectors[i].command,
          strlen(msg_dissectors[i].command)) == 0)
    {
      tvbuff_t *tvb_sub;

      col_append_sep_str(pinfo->cinfo, COL_INFO, ", ", msg_dissectors[i].command);

      tvb_sub = tvb_new_subset_remaining(tvb, offset);
      msg_dissectors[i].function(tvb_sub, pinfo, tree);
      return;
    }
  }

  /* no handler found */
  col_append_sep_str(pinfo->cinfo, COL_INFO, ", ", "[unknown command]");

  expert_add_info_format(pinfo, ti, PI_MALFORMED, PI_ERROR, "Unknown command");
}
//////////////////////////////////
////// dissect_bitcoin
////// Main disector entry point
////// 
////// Assemble multiple protocol data units and pass on to single pdu handler
//////
////// This is what gets called when we "decode as" bitcoin or by the heuristic decoder if it
////// detects we're a bitcoin pdu
//////////////////////////////////
static int
dissect_bitcoin(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  tcp_dissect_pdus(tvb, pinfo, tree, bitcoin_desegment, BITCOIN_HEADER_LENGTH,
      get_bitcoin_pdu_length, dissect_bitcoin_tcp_pdu);

  return tvb_reported_length(tvb);
}

//////////////////////////////////
////// dissect_bitcoin_heur
////// heuristic dissector entry point to detect we're a bitcoin pdu
////// 
//////////////////////////////////
static gboolean
dissect_bitcoin_heur(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  guint32 magic_number;
  conversation_t *conversation;

  if (tvb_length(tvb) < 4)
      return FALSE;

  magic_number = tvb_get_letohl(tvb, 0);
  if ((magic_number != BITCOIN_MAIN_MAGIC_NUMBER) &&
      (magic_number != BITCOIN_TESTNET_MAGIC_NUMBER) &&
      (magic_number != BITCOIN_TESTNET3_MAGIC_NUMBER) &&
      (magic_number != LITECOIN_MAIN_MAGIC_NUMBER) &&
      (magic_number != LITECOIN_TESTNET_MAGIC_NUMBER) &&
      (magic_number != DOGECOIN_MAIN_MAGIC_NUMBER))
     return FALSE;

  /* Ok: This connection should always use the bitcoin dissector */
  conversation = find_or_create_conversation(pinfo);
  conversation_set_dissector(conversation, bitcoin_handle);

  dissect_bitcoin(tvb, pinfo, tree, data);
  return TRUE;
}

//////////////////////////////////
////// proto_register_bitcoin(void)
////// register the dissector with wireshark
////// 
//////////////////////////////////
void
proto_register_bitcoin(void)
{
  static hf_register_info hf[] = {
    { &hf_bitcoin_magic,
      { "Packet magic", "bitcoin.magic", FT_UINT32, BASE_HEX, VALS(magic_types), 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_command,
      { "Command name", "bitcoin.command", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_length,
      { "Payload Length", "bitcoin.length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_checksum,
      { "Payload checksum", "bitcoin.checksum", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }
    },

    /* version message */
    { &hf_bitcoin_msg_version,
      { "Version message", "bitcoin.version", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_version,
      { "Protocol version", "bitcoin.version.version", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_services,
      { "Node services", "bitcoin.version.services", FT_UINT64, BASE_HEX, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_addr_me,
      { "Address of emmitting node", "bitcoin.version.addr_me", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_addr_you,
      { "Address as seen by the emitting node", "bitcoin.version.addr_you", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_timestamp,
      { "Node timestamp", "bitcoin.version.timestamp", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_nonce,
      { "Random nonce", "bitcoin.version.nonce", FT_UINT64, BASE_HEX, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_user_agent_length8,
      { "Length", "bitcoin.version.usr_agent.length", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_user_agent_length16,
      { "Length", "bitcoin.version.usr_agent.length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_user_agent_length32,
      { "Length", "bitcoin.version.usr_agent.length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_user_agent_length64,
      { "Length", "bitcoin.version.usr_agent.length", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_user_agent,
      { "User Agent string", "bitcoin.version.user_agent", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_version_start_height,
      { "Block start height", "bitcoin.version.start_height", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },

    /* addr message */
    { &hf_msg_addr_count8,
      { "Count", "bitcoin.addr.count", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_addr_count16,
      { "Count", "bitcoin.addr.count", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_addr_count32,
      { "Count", "bitcoin.addr.count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_addr_count64,
      { "Count", "bitcoin.addr.count", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_msg_addr,
      { "Address message", "bitcoin.addr", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_addr_address,
      { "Address", "bitcoin.addr.address", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_addr_timestamp,
      { "Address timestamp", "bitcoin.addr.timestamp", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0, NULL, HFILL }
    },

    /* inv message */
    { &hf_msg_inv_count8,
      { "Count", "bitcoin.inv.count", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_inv_count16,
      { "Count", "bitcoin.inv.count", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_inv_count32,
      { "Count", "bitcoin.inv.count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_inv_count64,
      { "Count", "bitcoin.inv.count", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_msg_inv,
      { "Inventory message", "bitcoin.inv", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_inv_type,
      { "Type", "bitcoin.inv.type", FT_UINT32, BASE_DEC, VALS(inv_types), 0x0, NULL, HFILL }
    },
    { &hf_msg_inv_hash,
      { "Data hash", "bitcoin.inv.hash", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },

    /* getdata message */
    { &hf_msg_getdata_count8,
      { "Count", "bitcoin.getdata.count", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getdata_count16,
      { "Count", "bitcoin.getdata.count", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getdata_count32,
      { "Count", "bitcoin.getdata.count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getdata_count64,
      { "Count", "bitcoin.getdata.count", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_msg_getdata,
      { "Getdata message", "bitcoin.getdata", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getdata_type,
      { "Type", "bitcoin.getdata.type", FT_UINT32, BASE_DEC, VALS(inv_types), 0x0, NULL, HFILL }
    },
    { &hf_msg_getdata_hash,
      { "Data hash", "bitcoin.getdata.hash", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },

    /* notfound message */
    { &hf_msg_notfound_count8,
      { "Count", "bitcoin.notfound.count", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_notfound_count16,
      { "Count", "bitcoin.notfound.count", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_notfound_count32,
      { "Count", "bitcoin.notfound.count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_notfound_count64,
      { "Count", "bitcoin.notfound.count", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_msg_notfound,
      { "Notfound message", "bitcoin.notfound", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_notfound_type,
      { "Type", "bitcoin.notfound.type", FT_UINT32, BASE_DEC, VALS(inv_types), 0x0, NULL, HFILL }
    },
    { &hf_msg_notfound_hash,
      { "Data hash", "bitcoin.notfound.hash", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },

    /* ping message */
    { &hf_bitcoin_msg_ping,
      { "Ping message", "bitcoin.ping", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_ping_nonce,
      { "Nonce", "bitcoin.ping.nonce", FT_UINT64, BASE_HEX, NULL, 0x0, NULL, HFILL }
    },
    /* pong message */
    { &hf_bitcoin_msg_pong,
      { "Pong message", "bitcoin.pong", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_pong_nonce,
      { "Nonce", "bitcoin.pong.nonce", FT_UINT64, BASE_HEX, NULL, 0x0, NULL, HFILL }
    },
    /* reject message */
    { &hf_bitcoin_msg_reject,
      { "Reject message", "bitcoin.reject", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_reject_command,
      { "Command", "bitcoin.reject.command", FT_STRINGZ, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_reject_code,
      { "Code", "bitcoin.reject.code", FT_UINT8, BASE_HEX, VALS(msg_reject_codes), 0x0, NULL, HFILL }
    },
    { &hf_msg_reject_reason,
      { "Reason", "bitcoin.reject.reason", FT_STRINGZ, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_reject_hash,
      { "Data hash", "bitcoin.reject.hash", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    /* alert message */
    { &hf_bitcoin_msg_alert,
      { "Alert message", "bitcoin.alert", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_message,
      { "Message", "bitcoin.alert.message", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_msg_length8,
      { "Length", "bitcoin.alert.msg.length", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_msg_length16,
      { "Length", "bitcoin.alert.msg.length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_msg_length32,
      { "Length", "bitcoin.alert.msg.length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_msg_length64,
      { "Length", "bitcoin.alert.msg.length", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_version,
      { "Version", "bitcoin.alert.message.version", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_relayuntil,
      { "Relay until", "bitcoin.alert.message.relayuntil", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_expiration,
      { "Expiration", "bitcoin.alert.message.expiration", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_id,
      { "Id", "bitcoin.alert.message.id", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_cancel,
      { "Cancel", "bitcoin.alert.message.cancel", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_cancel_set_count8,
      { "Cancel Set Count", "bitcoin.alert.cancelsetcount", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_cancel_set_count16,
      { "Cancel Set Count", "bitcoin.alert.cancelsetcount", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_cancel_set_count32,
      { "Cancel Set Count", "bitcoin.alert.cancelsetcount", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_cancel_set_count64,
      { "Cancel Set Count", "bitcoin.alert.cancelsetcount", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_cancel_set_item,
      { "Cancel Set Item", "bitcoin.alert.cancelsetitem", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_min_version,
      { "Min Version", "bitcoin.alert.message.minversion", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_max_version,
      { "Max Version", "bitcoin.alert.message.maxversion", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_subver_set_count8,
      { "SubVer Set Count", "bitcoin.alert.subversetcount", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_subver_set_count16,
      { "SubVer Set Count", "bitcoin.alert.subversetcount", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_subver_set_count32,
      { "SubVer Set Count", "bitcoin.alert.subversetcount", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_subver_set_count64,
      { "SubVer Set Count", "bitcoin.alert.subversetcount", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_subver_set_str_length8,
      { "Length", "bitcoin.alert.subversetstrlen", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_subver_set_str_length16,
      { "Length", "bitcoin.alert.subversetstrlen", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_subver_set_str_length32,
      { "Length", "bitcoin.alert.subversetstrlen", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_subver_set_str_length64,
      { "Length", "bitcoin.alert.subversetstrlen", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_subver_set_string,
      { "Subver", "bitcoin.alert.subversetstring", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_priority,
      { "Priority", "bitcoin.alert.message.priority", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_comment_length8,
      { "Length", "bitcoin.alert.commentlength", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_comment_length16,
      { "Length", "bitcoin.alert.commentlength", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_comment_length32,
      { "Length", "bitcoin.alert.commentlength", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_comment_length64,
      { "Length", "bitcoin.alert.commentlength", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_comment,
      { "Comment", "bitcoin.alert.comment", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_status_bar_length8,
      { "Length", "bitcoin.alert.statusbarlength", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_status_bar_length16,
      { "Length", "bitcoin.alert.statusbarlength", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_status_bar_length32,
      { "Length", "bitcoin.alert.statusbarlength", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_status_bar_length64,
      { "Length", "bitcoin.alert.statusbarlength", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_status_bar,
      { "Status Bar", "bitcoin.alert.statusbar", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_reserved_length8,
      { "Length", "bitcoin.alert.reservedlength", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_reserved_length16,
      { "Length", "bitcoin.alert.reservedlength", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_reserved_length32,
      { "Length", "bitcoin.alert.reservedlength", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_reserved_length64,
      { "Length", "bitcoin.alert.reservedlength", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_str_reserved,
      { "Reserved", "bitcoin.alert.reserved", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_signature,
      { "Signature", "bitcoin.alert.signature", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_signature_length8,
      { "Length", "bitcoin.alert.signature.length", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_signature_length16,
      { "Length", "bitcoin.alert.signature.length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_signature_length32,
      { "Length", "bitcoin.alert.signature.length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_signature_length64,
      { "Length", "bitcoin.alert.signature.length", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_alert_signature_data,
      { "Data", "bitcoin.alert.signature.data", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },

    /* getblocks message */
    { &hf_msg_getblocks_count8,
      { "Count", "bitcoin.getblocks.count", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getblocks_count16,
      { "Count", "bitcoin.getblocks.count", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getblocks_count32,
      { "Count", "bitcoin.getblocks.count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getblocks_count64,
      { "Count", "bitcoin.getblocks.count", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_msg_getblocks,
      { "Getblocks message", "bitcoin.getblocks", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getblocks_start,
      { "Starting hash", "bitcoin.getblocks.hash_start", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getblocks_stop,
      { "Stopping hash", "bitcoin.getblocks.hash_stop", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },

    /* getheaders message */
    { &hf_msg_getheaders_count8,
      { "Count", "bitcoin.getheaders.count", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getheaders_count16,
      { "Count", "bitcoin.getheaders.count", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getheaders_count32,
      { "Count", "bitcoin.getheaders.count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getheaders_count64,
      { "Count", "bitcoin.getheaders.count", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_msg_getheaders,
      { "Getheaders message", "bitcoin.getheaders", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getheaders_start,
      { "Starting hash", "bitcoin.getheaders.hash_start", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_getheaders_stop,
      { "Stopping hash", "bitcoin.getheaders.hash_stop", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },

    /* tx message */
    { &hf_bitcoin_msg_tx,
      { "Tx message", "bitcoin.tx", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },

    { &hf_msg_tx_version,
      { "Transaction version", "bitcoin.tx.version", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },

    /* tx message - input */
    { &hf_msg_tx_in_count8,
      { "Input Count", "bitcoin.tx.input_count", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_count16,
      { "Input Count", "bitcoin.tx.input_count", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_count32,
      { "Input Count", "bitcoin.tx.input_count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_count64,
      { "Input Count", "bitcoin.tx.input_count", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },

    { &hf_msg_tx_in,
      { "Transaction input", "bitcoin.tx.in", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_prev_output,
      { "Previous output", "bitcoin.tx.in.prev_output", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },

    { &hf_msg_tx_in_prev_outp_hash,
      { "Hash", "bitcoin.tx.in.prev_output.hash", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_prev_outp_index,
      { "Index", "bitcoin.tx.in.prev_output.index", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },

    { &hf_msg_tx_in_script8,
      { "Script Length", "bitcoin.tx.in.script_length", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_script16,
      { "Script Length", "bitcoin.tx.in.script_length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_script32,
      { "Script Length", "bitcoin.tx.in.script_length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_script64,
      { "Script Length", "bitcoin.tx.in.script_length", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_sig_script,
      { "Signature script", "bitcoin.tx.in.sig_script", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_in_seq,
      { "Sequence", "bitcoin.tx.in.seq", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },

    /* tx message - output */
    { &hf_msg_tx_out_count8,
      { "Output Count", "bitcoin.tx.output_count", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out_count16,
      { "Output Count", "bitcoin.tx.output_count", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out_count32,
      { "Output Count", "bitcoin.tx.output_count", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out_count64,
      { "Output Count", "bitcoin.tx.output_count", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out,
      { "Transaction output", "bitcoin.tx.out", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out_value,
      { "Value", "bitcoin.tx.out.value", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out_script8,
      { "Script Length", "bitcoin.tx.out.script_length", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out_script16,
      { "Script Length", "bitcoin.tx.out.script_length", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out_script32,
      { "Script Length", "bitcoin.tx.out.script_length", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out_script64,
      { "Script Length", "bitcoin.tx.out.script_length", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_tx_out_script,
      { "Script", "bitcoin.tx.out.script", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },

    { &hf_msg_tx_lock_time,
      { "Block lock time or block ID", "bitcoin.tx.lock_time", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },

    /* block message */
    { &hf_msg_block_transactions8,
      { "Number of transactions", "bitcoin.block.num_transactions", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_block_transactions16,
      { "Number of transactions", "bitcoin.tx.num_transactions", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_block_transactions32,
      { "Number of transactions", "bitcoin.tx.num_transactions", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_block_transactions64,
      { "Number of transactions", "bitcoin.tx.num_transactions", FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_bitcoin_msg_block,
      { "Block message", "bitcoin.block", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_block_version,
      { "Block version", "bitcoin.block.version", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_block_prev_block,
      { "Previous block", "bitcoin.block.prev_block", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_block_merkle_root,
      { "Merkle root", "bitcoin.block.merkle_root", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_block_time,
      { "Block timestamp", "bitcoin.block.timestamp", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_block_bits,
      { "Bits", "bitcoin.block.merkle_root", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }
    },
    { &hf_msg_block_nonce,
      { "Nonce", "bitcoin.block.nonce", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }
    },

    /* services */
    { &hf_services_network,
      { "Network node", "bitcoin.services.network", FT_BOOLEAN, 32, TFS(&tfs_set_notset), 0x1, NULL, HFILL }
    },

    /* address */
    { &hf_address_services,
      { "Node services", "bitcoin.address.services", FT_UINT64, BASE_HEX, NULL, 0x0, NULL, HFILL }
    },
    { &hf_address_address,
      { "Node address", "bitcoin.address.address", FT_IPv6, BASE_NONE, NULL, 0x0, NULL, HFILL }
    },
    { &hf_address_port,
      { "Node port", "bitcoin.address.port", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }
    }
  };

  static gint *ett[] = {
    &ett_bitcoin,
    &ett_bitcoin_msg,
    &ett_services,
    &ett_address,
    &ett_inv_list,
    &ett_getdata_list,
    &ett_notfound_list,
    &ett_getblocks_list,
    &ett_getheaders_list,
    &ett_tx_in_list,
    &ett_tx_in_outp,
    &ett_tx_out_list,
    &ett_ping,
    &ett_pong,
    &ett_reject,
    &ett_alert,
    &ett_alert_sig,
    &ett_alert_message,
  };

  module_t *bitcoin_module;

  proto_bitcoin = proto_register_protocol( "Bitcoin protocol", "Bitcoin",
      "bitcoin");

  proto_register_subtree_array(ett, array_length(ett));
  proto_register_field_array(proto_bitcoin, hf, array_length(hf));

  new_register_dissector("bitcoin", dissect_bitcoin, proto_bitcoin);

  bitcoin_module = prefs_register_protocol(proto_bitcoin, NULL);
  prefs_register_bool_preference(bitcoin_module, "desegment",
                                 "Desegment all Bitcoin messages spanning multiple TCP segments",
                                 "Whether the Bitcoin dissector should desegment all messages"
                                 " spanning multiple TCP segments",
                                 &bitcoin_desegment);

}

//////////////////////////////////
////// proto_reg_handoff_bitcoin(void)
////// initialize the dissector
////// 
//////////////////////////////////
void
proto_reg_handoff_bitcoin(void)
{
  bitcoin_handle = find_dissector("bitcoin");
  dissector_add_handle("tcp.port", bitcoin_handle);  /* for 'decode-as' */

  heur_dissector_add( "tcp", dissect_bitcoin_heur, proto_bitcoin);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
