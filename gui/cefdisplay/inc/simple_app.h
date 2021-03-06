// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_

#include "include/cef_app.h"

#include "gui_handler.h"
#include "osr_handler.h"

class THttpServer;

// Implement application-level callbacks for the browser process.
class SimpleApp : public CefApp, public CefBrowserProcessHandler /*, public CefRenderProcessHandler */ {
protected:
   std::string fCefMain;   ///<! extra executable used for additional processes
   bool fLastBatch{false}; ///<! indicate if last started window was batch
   std::string fFirstUrl;     ///<! first URL to open
   bool fFirstBatch{false};   ///<! indicate batch mode
   CefRect fFirstRect;        ///<! original width

   CefRefPtr<OsrHandler> fOsrHandler; ///<! batch-mode handler
   bool fUseViewes{false};            ///<! is views are used
   CefRefPtr<GuiHandler> fGuiHandler; ///<! normal handler

public:
   SimpleApp(const std::string &cef_main, const std::string &url = "", bool isbatch = false, int width = 0, int height = 0);
   virtual ~SimpleApp();

   // CefApp methods:
   virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE { return this; }
   // virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE { return this; }

   virtual void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) OVERRIDE;

   // CefBrowserProcessHandler methods:
   virtual void OnContextInitialized() OVERRIDE;

   virtual void
   OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) OVERRIDE;

   virtual void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) OVERRIDE;

   void StartWindow(const std::string &url, bool batch, CefRect &rect);

   // CefRenderProcessHandler methods
   // virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
   //                              CefRefPtr<CefV8Context> context) OVERRIDE;

private:
   // Include the default reference counting implementation.
   IMPLEMENT_REFCOUNTING(SimpleApp);
   DISALLOW_COPY_AND_ASSIGN(SimpleApp);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
