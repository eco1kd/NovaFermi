#include <linux/kernel.h>
#include <linux/module.h>

#include "nv_fermi_drv.h"

#define NV_CCB_ACCESS_I2C 5
#define NV_CCB_ACCESS_DP_AUX 6
#define NV_CCB_ACCESS_SKIP 0xff

struct nv_ccb_entry {
  u8 access_method;
  u8 phys_port;
  u8 speed;
  u8 dp_hybrid;
};

static void nv_fermi_ccb_decode(const u8 *e, struct nv_ccb_entry *out) {
  out->access_method = e[3];
  out->phys_port = e[0] & 0x0f;
  out->speed = (e[0] >> 4) & 0x0f;
  out->dp_hybrid = e[1] & 0x01;
}

static int nv_fermi_ccb_parse(struct nv_fermi_priv *priv) {
  u16 off = priv->dcb.i2c_table_off;
  const u8 *p;
  u8 header_len, entry_count, entry_len;
  int i;

  if (!off || (size_t)off + 4 > priv->vbios_len) {
    pr_err(DRV_NAME ": CCB table offset invalid, cannot parse\n");
    return -ENODEV;
  }

  p = priv->vbios + off;
  header_len = p[1];
  entry_count = p[2];
  entry_len = p[3];

  pr_info(DRV_NAME ": CCB table: header_len=%u entry_count=%u entry_len=%u\n",
          header_len, entry_count, entry_len);

  if (entry_len < 4) {
    pr_err(DRV_NAME
           ": CCB entry_len=%u smaller than expected 4, refusing to parse\n",
           entry_len);
    return -EINVAL;
  }

  if ((size_t)off + header_len + (size_t)entry_count * entry_len >
      priv->vbios_len) {
    pr_err(DRV_NAME ": CCB table entries run past the end of the VBIOS, "
                    "refusing to parse\n");
    return -EINVAL;
  }

  for (i = 0; i < entry_count; i++) {
    const u8 *e = p + header_len + (size_t)i * entry_len;
    struct nv_ccb_entry entry;

    nv_fermi_ccb_decode(e, &entry);

    if (entry.access_method == NV_CCB_ACCESS_SKIP) {
      pr_info(DRV_NAME ": CCB entry[%d]: skip (raw %02x %02x %02x %02x)\n", i,
              e[0], e[1], e[2], e[3]);
      continue;
    }

    if (entry.access_method == NV_CCB_ACCESS_I2C) {
      pr_info(DRV_NAME ": CCB entry[%d]: I2C phys_port=%u speed=%u (raw %02x "
                       "%02x %02x %02x)\n",
              i, entry.phys_port, entry.speed, e[0], e[1], e[2], e[3]);
    } else if (entry.access_method == NV_CCB_ACCESS_DP_AUX) {
      pr_info(DRV_NAME ": CCB entry[%d]: DP-AUX port=%u hybrid=%u (raw %02x "
                       "%02x %02x %02x)\n",
              i, entry.phys_port, entry.dp_hybrid, e[0], e[1], e[2], e[3]);
    } else {
      pr_warn(DRV_NAME ": CCB entry[%d]: unknown access_method=0x%02x (raw "
                       "%02x %02x %02x %02x)\n",
              i, entry.access_method, e[0], e[1], e[2], e[3]);
    }
  }

  return 0;
}

static int nv_fermi_i2c_devices_parse(struct nv_fermi_priv *priv) {
  u16 off = priv->dcb.i2c_devices_table_off;
  const u8 *p;
  u8 header_len, entry_count, entry_len;
  int i;

  if (!off || (size_t)off + 4 > priv->vbios_len) {
    pr_warn(DRV_NAME ": I2C Devices table offset not available, skipping\n");
    return -ENODEV;
  }

  p = priv->vbios + off;
  header_len = p[1];
  entry_count = p[2];
  entry_len = p[3];

  if (entry_len < 4) {
    pr_err(DRV_NAME ": I2C Devices entry_len=%u smaller than expected 4, "
                    "refusing to parse\n",
           entry_len);
    return -EINVAL;
  }

  if ((size_t)off + header_len + (size_t)entry_count * entry_len >
      priv->vbios_len) {
    pr_err(DRV_NAME ": I2C Devices table entries run past the end of the "
                    "VBIOS, refusing to parse\n");
    return -EINVAL;
  }

  for (i = 0; i < entry_count; i++) {
    const u8 *e = p + header_len + (size_t)i * entry_len;
    u8 type = e[0];
    u8 addr = e[1];

    if (type == 0xff) {
      pr_info(DRV_NAME ": I2C device[%d]: skip\n", i);
      continue;
    }

    pr_info(DRV_NAME ": I2C device[%d]: type=0x%02x i2c_addr=0x%02x (raw %02x "
                     "%02x %02x %02x)\n",
            i, type, addr, e[0], e[1], e[2], e[3]);
  }

  return 0;
}

int nv_fermi_i2c_init(struct nv_fermi_priv *priv) {
  if (!priv->dcb_valid) {
    pr_warn(DRV_NAME ": DCB not parsed, skipping I2C init\n");
    return -ENODEV;
  }

  if (nv_fermi_ccb_parse(priv))
    pr_warn(DRV_NAME ": CCB table parse failed\n");

  nv_fermi_i2c_devices_parse(priv);

  pr_info(DRV_NAME ": I2C bit-bang engine not yet implemented\n");
  return 0;
}