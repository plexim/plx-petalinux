# RTbox CE/1/2/3 Angular frontend

## High-level overview

This page is an Angular web frontend for all RTBoxes. It communicates with an RTBox over the CGI interfaces and the
Xml/JsonRpc server on port 9998 using Json. When no RTBox is available/connected, development can be done with automated fake data (see
below for details). In short, Angular imposes a rather rigid component-based structure onto the project. This is a good
thing. A component is an element of the view, possibly very little in scope, with a one-to-one mapping between HTML
template and a class instance that manages state and events. Style sheets are also local to a view. Code is written in
Typescript, a strict superset of Javascript. Typescript's main selling point is a static type checker that is completely
opt-in: as any valid Javascript code is valid Typescript, type-annotations are hence meant to help development, but
don't restrict the runtime: building the page transpiles to pure, non-annotated Javascript.

## Getting started

Make sure you install NodeJS (with Homebrew) and Angular (as a node package). Consider installing VSCode. Visual Studio 
Code offers some nice development features including advanced debugging. See also the corresponding
[Wiki page](https://intranet.plexim.com/mediawiki/index.php/Angular_on_MacOS) for more setup instructions.
After you have cloned the repository, run `npm install` to pull in all dependencies (bootstrap etc.) To begin development,
run `ng serve` and inspect the result at localhost:4200. When `ng serve` is running, any changes you make are immediately
compiled into an updated version of the webpage. Note that `ng serve` is a pure development tool. A production build is
made with `ng build --prod`.

## RTBox choice

The page can be configured for RTBoxes CE, 1, 2 or 3. This is done in `src/app/app.module.ts`, look for the line:
```typescript
    { provide: RTBOX_TYPE, useValue: 2 },
```
and adjust the integer (0 is for the RTBox CE). The project contains templates for each product:
- app.module.ts.rtbox1
- app.module.ts.rtbox2
- app.module.ts.rtbox3
- app.module.ts.rtboxce

For development you might want to duplicate one of these files and rename it to `app.module.ts`.
If you build the page for production use, don't forget to adjust this parameter as well or duplicate and rename the correct 
file. The number is a compile-time constant used to easily and efficiently dispatch over different view
behaviour and options (it is injected into all components that have different behavior for different boxes). While the
RTBox type can also be obtained through XmlRpc/JsonRpc, using this would make large parts of the frontend depend on a possibly
delayed HTTP answer, which is unnecessarilly complicated to deal with and slow. Instead, one run-time check is performed
to compare the configured and actual RTBox type - if they differ, an `alert`-warning is shown.

If you don't want to connect to a box in your network, use the fake data by settig the appropriate "`Stub`" services in
`src/app/app.module.ts`:
```typescript
    { provide : RtboxCommunicationService, useClass: RtboxCommunicationStubService },
    { provide : RtboxCgiService, useClass: RtboxCgiStubService },
```
Otherwise, use
```typescript
    { provide : RtboxCommunicationService, useClass: RtboxJsonRpcService },
    { provide : RtboxCgiService, useClass: RtboxCgiService },
```
In addition, when developing, you need to customize the file
`./proxy.conf.json`. This instructs the development server where to redirect certain HTTP requests. For a box with the
IP address 10.0.0.157, use
```javascript
{
  "/cgi-bin/*": {
    "target": "http://10.0.0.157",
    "logLevel": "info",
    "secure": false
  },
  "/rpc2/*": {
    "target": "http://10.0.0.157:9998",
    "logLevel": "info",
    "secure": false
  }
}
```
If you don't receive any XmlJson responses at all, make sure the firmware on the box is recent enough to have the merge
[1a1d92da](https://gitlab.plexim.com/hil/hil_sw/-/commit/1a1d92da4d66943486385b6f18e44a3c6d141c9e). 
Also see the older fix [6db1909](https://gitlab.plexim.com/hil/hil_sw/-/commit/6db1909068aa952e5cd139c54eace79129ef0bdf) 
(merged into masterwith [a3846f86](https://gitlab.plexim.com/hil/hil_sw/-/commit/a3846f8684e95abacc8897156588afd7968c161d)
on September 23, 2020), see [issue 5254](https://gitlab.plexim.com/hil/hil_sw/-/issues/5254) for a description of the root cause. 
If this is not the case, and there is no way to update the firmware, edit the above `proxy.conf.json` such that the XmlRpc
requests are sent to localhost:9998, and then start the following python script to circumvent the issue.
```python
import http.server
import http.client
import socketserver

LOCAL_PORT = 9998
TARGET_PORT = 9998
TARGET = '10.0.0.237'

client = http.client.HTTPConnection(TARGET, TARGET_PORT)

class ContentLengthHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        cl = self.headers.get('content-length')
        self.headers['Content-Length'] = cl
        body = self.rfile.read(int(cl))

        client.request("POST", "rpc2", body)

        resp = client.getresponse()
        self.send_response(resp.status)
        self.end_headers()
        self.wfile.write(resp.read())

httpd = socketserver.TCPServer(("", LOCAL_PORT), ContentLengthHandler)
httpd.serve_forever()
```

## Development

It helps to have an idea of the following concepts. Note that all Angular-related topics, including rxjs, is very well
covered in this (German) [book](https://angular-buch.com/).

- XmlRpc interface to the scopeserver running on the RTBox. Have a look at
  `scopeserver/files/xml-rpc/RtBoxXmlRpcServer.cpp` in `hil_sw/petalinux/rtbox2/project-spec/meta-user/recipes-apps/` to
  see what functions can be called.
- The CGI interface, see `hil-webserver/files/cgihandler.pl` in
  `hil_sw/petalinux/rtbox2/project-spec/meta-user/recipes-apps/`.
- The rxjs library and its operators, including the higher-order mapping functions (e.g. `flatMap`, `switchMap`). This
  is crucial to work with `Observable`s, a future-like result type that Angular uses for server responses. Especially
  when there is a dependency between `Observable`s, you want to understand the concepts before changing anything.
- Angular's two-way binding for data and events. Never forget brackets around a property if you want to have it handled
  dynamically (if you don't understand this, just wait -- you will certainly make this mistake at some point).
- Dependency injection used by Angular: if you want to use an injectable object, e.g. a service for communicating with
  the RTBox, you declaratively add an instance to the constructor parameters of a component. Angular then does the rest.
  You would _never_ manually create that instance and/or manually invoke any constructor.
- Passing data from parent to child component through `@Input`-decorated member variables


## Graphics

All graphics so far have been realised as svg, in two ways. _Inline_ svg means manually editing the svg tags within an
Angular component. This is quite versatile and powerful and allows for styling with component-specific CSS, but it's
only efficient for primitive graphical content: as soon as we want gradients, shadows and such, it takes too much time.
The other option is using a graphical svg editor that exports distinct files. This doesn't comfortably allow for styling
with a separate style sheet, but arranging, modifying and creating objects is much simpler than doing it by hand. It's
still advantageous to edit the files by hand from time to time, just don't forget that this is still possible after
exporting something from your visual svg editor.

## Inkscape

When editing svg graphics with Inkscape, I found the following useful.

- Essential Inkscape menus (keep the at the right pane)
  * Fill and stroke
  * Align and distribute (let's you automatically align items horizontally or distribute them equidistantly)
  * Xml editor - to select hidden items
- Essential Inkscape keyboard shortcuts
  * Group/Ungroup: Cmd+G/Cmd+Shift+G
  * Resize to content: Cmd+Shift+R
  * Invert selection (e.g. to remove everything but a selected item): !
  * Resize an item preserving the aspect ratio: Ctrl+click/drag on the resize arrows
  * Toggle snapping: %
  * Cycle through items (can select non-clickable/hidden items): Tab/Shift+Tab
- Scaling: I usually want to preserve stroke widths and radii when scaling something like a rectangle. There are two
  buttons in the second row at the top that should _not_ be enabled (Tooltip 'When scaling objects, scale the stroke
  width by the same proportion' etc.).
- Composite Svg from individual ones: Cmd+I lets you import images. Make sure you select 'Link the SVG file in an image
  tag'. This inserts a tag that only refers to the imported file. We want this, because it lets us change the individual
  svg and have the changes automatically propagate to the composite graphics.
- Save as: "Optimized Svg"
  * This removes all unnecessary (Inkscape-related) metadata and reduces the filesize
  * After saving as "Optimized SVG" and quitting the application, Inkscape complains about loosing data when the graphic
    isn't saved in an Inkscape-enhanced svg format. I think as long as we don't use non-standard-svg features like
    layers and such, we don't loose anything. So far, I always chose 'Close without saving' and reopening the "optimized
    SVG" brought back everything needed.
