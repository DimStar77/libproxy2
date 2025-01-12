/* config-windows.c
 *
 * Copyright 2022-2023 The Libproxy Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <windows.h>
#include <winreg.h>
#include <libpeas/peas.h>

#include "config-windows.h"

#include "px-plugin-config.h"
#include "px-manager.h"

#define W32REG_OFFSET_PAC (1 << 2)
#define W32REG_OFFSET_WPAD (1 << 3)
#define W32REG_BASEKEY "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define W32REG_BUFFLEN 1024

struct _PxConfigWindows {
  GObject parent_instance;
};

static void px_config_iface_init (PxConfigInterface *iface);
void peas_register_types (PeasObjectModule *module);

G_DEFINE_FINAL_TYPE_WITH_CODE (PxConfigWindows,
                               px_config_windows,
                               G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (PX_TYPE_CONFIG, px_config_iface_init))

static void
px_config_windows_init (PxConfigWindows *self)
{
}

static void
px_config_windows_class_init (PxConfigWindowsClass *klass)
{
}

static gboolean
px_config_windows_is_available (PxConfig *self)
{
  return TRUE;
}

static gboolean
get_registry (const char  *key,
              const char  *name,
              char       **sval,
              guint32     *slen,
              guint32     *ival)
{
  HKEY hkey;
  LONG result;
  DWORD type;
  DWORD buflen = W32REG_BUFFLEN;
  BYTE buffer[W32REG_BUFFLEN];

  if (sval && ival)
    return FALSE;

  if (RegOpenKeyExA (HKEY_CURRENT_USER, key, 0, KEY_READ, &hkey) != ERROR_SUCCESS)
    return FALSE;

  result = RegQueryValueExA (hkey, name, NULL, &type, buffer, &buflen);
  RegCloseKey (hkey);

  if (result != ERROR_SUCCESS)
    return FALSE;

  switch (type) {
    case REG_BINARY:
    case REG_EXPAND_SZ:
    case REG_SZ:
      if (!sval)
        return FALSE;
      if (slen)
        *slen = buflen;

      *sval = g_malloc0 (buflen);
      return memcpy (*sval, buffer, buflen) != NULL;
    case REG_DWORD:
      if (ival)
        return memcpy (ival, buffer, buflen < sizeof (guint32) ? buflen : sizeof (guint32)) != NULL;
    default:
      break;
  }

  return FALSE;
}

static GHashTable *
parse_manual (char *manual)
{
  g_auto (GStrv) split = NULL;
  GHashTable *ret = g_hash_table_new (g_str_hash, g_str_equal);

  /* We have to check for two formats:
   * - 1.2.3.4:8080
   * - ftp=1.2.4.5:8080;https=1.2.3.4:8080
   */

  split = g_strsplit (manual, ";", -1);
  for (int idx = 0; idx < g_strv_length (split); idx++) {
    if (!strchr (split[idx], '=')) {
      g_hash_table_insert (ret, (char *)"http", g_strdup_printf ("http://%s", split[idx]));
    } else {
      g_auto (GStrv) split_kv = g_strsplit (split[idx], "=", -1);

      if (g_strv_length (split_kv) == 2) {
        g_hash_table_insert (ret, g_strdup (split_kv[0]), g_strdup_printf ("%s://%s", split_kv[0], split_kv[1]));
      }
    }
  }

  return ret;
}

static gboolean
is_enabled (char type)
{
  g_autofree char *data = NULL;
  guint32 dlen = 0;
  gboolean result = FALSE;

  if (!get_registry (W32REG_BASEKEY "\\Connections", "DefaultConnectionSettings", &data, &dlen, NULL))
    return FALSE;

  if (dlen >= 9)
    result = (data[8] & type) == type;

  return result;
}

static void
px_config_windows_get_config (PxConfig     *self,
                              GUri         *uri,
                              GStrvBuilder *builder)
{
  char *tmp = NULL;
  guint32 enabled = 0;

  if (get_registry (W32REG_BASEKEY, "ProxyOverride", &tmp, NULL, NULL)) {
    const char *host = g_uri_get_host (uri);

    if (g_strcmp0 (tmp, "<local>") == 0 && g_strcmp0 (host, "127.0.0.1") == 0)
      return;
  }

  /* WPAD */
  if (is_enabled (W32REG_OFFSET_WPAD)) {
    px_strv_builder_add_proxy (builder, "wpad://");
    return;
  }

  /* PAC */
  if (is_enabled (W32REG_OFFSET_PAC) && get_registry (W32REG_BASEKEY, "AutoConfigURL", &tmp, NULL, NULL)) {
    g_autofree char *pac_uri = g_strconcat ("pac+", tmp, NULL);
    GUri *ac_uri = g_uri_parse (tmp, G_URI_FLAGS_PARSE_RELAXED, NULL);

    if (ac_uri) {
      px_strv_builder_add_proxy (builder, pac_uri);
      return;
    }
  }

  /* Manual proxy */
  if (get_registry (W32REG_BASEKEY, "ProxyEnable", NULL, NULL, &enabled) && enabled && get_registry (W32REG_BASEKEY, "ProxyServer", &tmp, NULL, NULL)) {
    g_autoptr (GHashTable) table = parse_manual (tmp);
    const char *scheme = g_uri_get_scheme (uri);

    if (table) {
      char *ret = g_hash_table_lookup (table, scheme);
      if (ret) {
        px_strv_builder_add_proxy (builder, ret);
        return;
      }

      ret = g_hash_table_lookup (table, "http");
      if (ret) {
        px_strv_builder_add_proxy (builder, ret);
        return;
      }

      ret = g_hash_table_lookup (table, "socks");
      if (ret) {
        px_strv_builder_add_proxy (builder, ret);
        return;
      }
    }
  }
}

static void
px_config_iface_init (PxConfigInterface *iface)
{
  iface->is_available = px_config_windows_is_available;
  iface->get_config = px_config_windows_get_config;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              PX_TYPE_CONFIG,
                                              PX_CONFIG_TYPE_WINDOWS);
}
