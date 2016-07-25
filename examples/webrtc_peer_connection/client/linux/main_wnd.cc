/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/examples/peerconnection/client/linux/main_wnd.h"

#include <stddef.h>
#include <string>

#include "webrtc/examples/peerconnection/client/defaults.h"
#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/stringutils.h"

using rtc::sprintfn;

//
// SessionManager implementation.
//

SessionManager::SessionManager(const char* server, int port, bool autoconnect,
                       bool autocall)
    : callback_(NULL),server_(server), autoconnect_(autoconnect), autocall_(autocall) {
  char buffer[10];
  sprintfn(buffer, sizeof(buffer), "%i", port);
  port_ = buffer;
}

SessionManager::~SessionManager() {
}

void SessionManager::RegisterObserver(MainConsoleCallback* callback) {
  callback_ = callback;
}


void SessionManager::Message(std::string text) {
  printf(text);
}

MainConsole::UI SessionManager::current_ui() {
  if (vbox_)
    return CONNECT_TO_SERVER;

  if (peer_list_)
    return LIST_PEERS;

  return STREAMING;
}

void SessionManager::QueueUIThreadCallback(int msg_id, void* data) {
  g_idle_add(HandleUIThreadCallback,
             new UIThreadCallbackData(callback_, msg_id, data));
}

bool SessionManager::Start(){
    SwitchToConnectUI();
    return true;
}

void SessionManager::SwitchToConnectUI() {
  LOG(INFO) << __FUNCTION__;

//  if (peer_list_) {
//    peer_list_ = NULL;
//  }

  server_ = "localhost";
  port_ = "8888";

  if (autoconnect_)
    OnStart();
}

void SessionManager::SwitchToPeerList(const Peers& peers) {
  LOG(INFO) << __FUNCTION__;

  if (!peer_list_) {
    gtk_container_set_border_width(GTK_CONTAINER(window_), 0);
    if (vbox_) {
      gtk_widget_destroy(vbox_);
      vbox_ = NULL;
      server_edit_ = NULL;
      port_edit_ = NULL;
    } else if (draw_area_) {
      gtk_widget_destroy(draw_area_);
      draw_area_ = NULL;
      draw_buffer_.reset();
    }

    peer_list_ = gtk_tree_view_new();
    g_signal_connect(peer_list_, "row-activated",
                     G_CALLBACK(OnRowActivatedCallback), this);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(peer_list_), FALSE);
    InitializeList(peer_list_);
    gtk_container_add(GTK_CONTAINER(window_), peer_list_);
    gtk_widget_show_all(window_);
  } else {
    GtkListStore* store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(peer_list_)));
    gtk_list_store_clear(store);
  }

  AddToList(peer_list_, "List of currently connected peers:", -1);
  for (Peers::const_iterator i = peers.begin(); i != peers.end(); ++i)
    AddToList(peer_list_, i->second.c_str(), i->first);

  if (autocall_ && peers.begin() != peers.end())
    g_idle_add(SimulateLastRowActivated, peer_list_);
}

void SessionManager::SwitchToStreamingUI() {
  LOG(INFO) << __FUNCTION__;

  ASSERT(draw_area_ == NULL);

  gtk_container_set_border_width(GTK_CONTAINER(window_), 0);
  if (peer_list_) {
    gtk_widget_destroy(peer_list_);
    peer_list_ = NULL;
  }

  draw_area_ = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(window_), draw_area_);

  gtk_widget_show_all(window_);
}

void SessionManager::OnDestroyed() {
  callback_->Close();
}

void SessionManager::OnStart(GtkWidget* widget) {
  int port = port_.length() ? atoi(port_.c_str()) : 0;
  callback_->StartLogin(server_, port);
}

void SessionManager::OnRowActivated(GtkTreeView* tree_view, GtkTreePath* path,
                                GtkTreeViewColumn* column) {
  ASSERT(peer_list_ != NULL);
  GtkTreeIter iter;
  GtkTreeModel* model;
  GtkTreeSelection* selection =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
     char* text;
     int id = -1;
     gtk_tree_model_get(model, &iter, 0, &text, 1, &id,  -1);
     if (id != -1)
       callback_->ConnectToPeer(id);
     g_free(text);
  }
}
