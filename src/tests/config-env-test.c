/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2022-2023 Jan-Michael Brummer <jan.brummer@tabos.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 ******************************************************************************/

#include "px-manager.h"

#include "px-manager-helper.h"

#include <glib.h>

typedef struct {
  const char *env;
  const char *proxy;
  const char *url;
  gboolean success;
} ConfigEnvTest;

static const ConfigEnvTest config_env_test_set[] = {
  { "HTTP_PROXY", "http://127.0.0.1:8080", "https://www.example.com", TRUE},
  { "HTTP_PROXY", "http://127.0.0.1:8080", "http://www.example.com", TRUE},
  { "HTTP_PROXY", "http://127.0.0.1:8080", "ftp://www.example.com", TRUE},
  { "HTTPS_PROXY", "http://127.0.0.1:8080", "https://www.example.com", TRUE},
  { "HTTPS_PROXY", "http://127.0.0.1:8080", "http://www.example.com", FALSE},
  { "HTTPS_PROXY", "http://127.0.0.1:8080", "ftp://www.example.com", FALSE},
  { "FTP_PROXY", "http://127.0.0.1:8080", "https://www.example.com", FALSE},
  { "FTP_PROXY", "http://127.0.0.1:8080", "http://www.example.com", FALSE},
  { "FTP_PROXY", "http://127.0.0.1:8080", "ftp://www.example.com", TRUE},
};

static void
test_config_env (void)
{
  int idx;

  /* Overwrite libproxy configuration module */
  g_setenv ("PX_CONFIG_MODULE", "Environment", TRUE);

  for (idx = 0; idx < G_N_ELEMENTS (config_env_test_set); idx++) {
    g_autoptr (PxManager) manager = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GUri) uri = NULL;
    g_auto (GStrv) config = NULL;
    ConfigEnvTest test = config_env_test_set[idx];

    /* Set proxy environment variable. Must be done before px_test_manager_new()! */
    g_setenv (test.env, test.proxy, TRUE);

    manager = px_test_manager_new ();
    g_clear_error (&error);

    uri = g_uri_parse (test.url, G_URI_FLAGS_PARSE_RELAXED, &error);
    if (!uri) {
      g_warning ("Could not parse url '%s': %s", test.url, error ? error->message : "");
      g_assert_not_reached ();
    }

    config = px_manager_get_configuration (manager, uri, &error);
    if (config) {
      if (test.success)
        g_assert_cmpstr (config[0], ==, test.proxy);
      else
        g_assert_cmpstr (config[0], !=, test.proxy);
    }

    g_unsetenv (test.env);
    g_clear_object (&manager);
  }
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/config/env", test_config_env);

  return g_test_run ();
}
