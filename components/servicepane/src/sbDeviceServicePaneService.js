/* vim: set ts=2 sw=2 expandtab : */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

/**
 * \file sbDeviceServicePane.js
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const CONTRACTID = "@getnightingale.com/servicepane/device;1";

const SP = "http://getnightingale.com/rdf/servicepane#";
const LSP = 'http://getnightingale.com/rdf/library-servicepane#';
const DEVICESP_NS = "http://getnightingale.com/rdf/device-servicepane#";

const URN_PREFIX_DEVICE = "urn:device:";
const DEVICE_NODE_WEIGHT = -2;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/DOMUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/DebugUtils.jsm");

/**
 * Given the arguments var of a function, dump the
 * name of the function and the parameters provided
 */
function logcall(parentArgs) {
  dump("\n");
  dump(parentArgs.callee.name + "(");
  for (var i = 0; i < parentArgs.length; i++) {
    dump(parentArgs[i]);
    if (i < parentArgs.length - 1) {
      dump(', ');
    }
  }
  dump(")\n");
}



/**
 * /class sbDeviceServicePane
 * /brief Provides device related nodes for the service pane
 * \sa sbIServicePaneService sbIDeviceServicePaneService
 */
function sbDeviceServicePane() {
  this._servicePane = null;
  this._libraryServicePane = null;

  // use the default stringbundle to translate tree nodes
  this.stringbundle = null;

  this.log = DebugUtils.generateLogFunction("DeviceServicePaneService");
}

//////////////////////////
// XPCOM Support        //
//////////////////////////
sbDeviceServicePane.prototype.QueryInterface =
  XPCOMUtils.generateQI([Ci.sbIServicePaneModule,
                         Ci.sbIServicePaneMutationListener,
                         Ci.sbIDeviceServicePaneService]);
sbDeviceServicePane.prototype.classDescription =
  "Nightingale Device Service Pane Service";
sbDeviceServicePane.prototype.classID =
  Components.ID("{845c31ee-c30e-4fb6-9667-0b10e58c7069}");
sbDeviceServicePane.prototype.contractID =
  CONTRACTID;
sbDeviceServicePane.prototype._xpcom_categories = [
  {
    category: 'service-pane',
    entry: 'device', // we want this to load first
    value: CONTRACTID
  }];

//////////////////////////
// sbIServicePaneModule //
//////////////////////////

sbDeviceServicePane.prototype.servicePaneInit =
function sbDeviceServicePane_servicePaneInit(sps) {
  //logcall(arguments);

  // keep track of the service pane services
  this._servicePane = sps;
  this._libraryServicePane = Cc["@getnightingale.com/servicepane/library;1"]
                               .getService(Ci.sbILibraryServicePaneService);

  // load the device context menu document
  this._deviceContextMenuDoc =
        DOMUtils.loadDocument
          ("chrome://nightingale/content/xul/device/deviceContextMenu.xul");

  // cache the device manager
  this._deviceMgr = Cc["@getnightingale.com/Nightingale/DeviceManager;2"]
                      .getService(Ci.sbIDeviceManager2);
}

sbDeviceServicePane.prototype.shutdown =
function sbDeviceServicePane_shutdown() {
  // release object references
  this._servicePane.root.removeMutationListener(this);
  this._servicePane = null;
  if (this._deviceGroupNode)
    this._deviceGroupNode.removeMutationListener(this);
  this._deviceGroupNode = null;
  this._libraryServicePane = null;
  this._deviceContextMenuDoc = null;
  this._deviceMgr = null;
}

sbDeviceServicePane.prototype.fillContextMenu =
function sbDeviceServicePane_fillContextMenu(aNode, aContextMenu, aParentWindow) {
  // Get the node device ID.  Do nothing if not a device node.
  var deviceID = aNode.getAttributeNS(DEVICESP_NS, "device-id");
  if (!deviceID)
    return;

  // Do nothing if not set to fill with the default device context menu items.
  var fillDefaultContextMenu = aNode.getAttributeNS(DEVICESP_NS,
                                                    "fillDefaultContextMenu");
  if (fillDefaultContextMenu != "true")
    return;

  // Get the device node type.
  var deviceNodeType = aNode.getAttributeNS(DEVICESP_NS, "deviceNodeType");

  // Import device context menu items into the context menu.
  if (deviceNodeType == "device") {
    DOMUtils.importChildElements(aContextMenu,
                                 this._deviceContextMenuDoc,
                                 "device_context_menu_items",
                                 { "device-id": deviceID,
                                   "service_pane_node_id": aNode.id });
  } else if (deviceNodeType == "library") {
    DOMUtils.importChildElements(aContextMenu,
                                 this._deviceContextMenuDoc,
                                 "device_library_context_menu_items",
                                 { "device-id": deviceID,
                                   "service_pane_node_id": aNode.id });
  }
}

sbDeviceServicePane.prototype.fillNewItemMenu =
function sbDeviceServicePane_fillNewItemMenu(aNode, aContextMenu, aParentWindow) {
}

sbDeviceServicePane.prototype.onSelectionChanged =
function sbDeviceServicePane_onSelectionChanged(aNode, aContainer, aParentWindow) {
}

sbDeviceServicePane.prototype.canDrop =
function sbDeviceServicePane_canDrop(aNode, aDragSession, aOrientation, aWindow) {
  return false;
}

sbDeviceServicePane.prototype.onDrop =
function sbDeviceServicePane_onDrop(aNode, aDragSession, aOrientation, aWindow) {
}

sbDeviceServicePane.prototype.onDragGesture =
function sbDeviceServicePane_onDragGesture(aNode, aTransferable) {
  return false;
}


/**
 * Called when the user is about to attempt to rename a device node
 */
sbDeviceServicePane.prototype.onBeforeRename =
function sbDeviceServicePane_onBeforeRename(aNode) {
}

/**
 * Called when the user has attempted to rename a device node
 */
sbDeviceServicePane.prototype.onRename =
function sbDeviceServicePane_onRename(aNode, aNewName) {
}


//////////////////////////////////
// sbIDeviceServicePaneService //
//////////////////////////////////

sbDeviceServicePane.prototype.createNodeForDevice =
function sbDeviceServicePane_createNodeForDevice(aDevice, aDeviceIdentifier) {
  //logcall(arguments);
  this.log("createNodeForDevice");

  // Make sure the devices group exists first
  let devicesNode = this._ensureDevicesGroupExists();

  // Get the Node.
  var id = this._deviceURN(aDevice, aDeviceIdentifier);
  var node = this._servicePane.getNode(id);
  if (!node) {
    // Create the node
    node = this._servicePane.createNode();
    node.id = id;
  }

  // Refresh the information just in case it is supposed to change
  node.contractid = CONTRACTID;
  node.setAttributeNS(SP, "Weight", DEVICE_NODE_WEIGHT);
  node.editable = false;

  // Add the node
  if (!node.parentNode) {
    this.log("\tNo parentNode, appending to devices group node");
    devicesNode.appendChild(node);
  }

  return node;
}

sbDeviceServicePane.prototype.createNodeForDevice2 =
function sbDeviceServicePane_createNodeForDevice2(aDevice, aEjectable) {
  this.log("createNodeForDevice2");

  // Make sure the devices group exists first
  let devicesNode = this._ensureDevicesGroupExists();

  // Get the Node.
  var id = this._deviceURN2(aDevice);
  
  var node = this._servicePane.getNode(id);
  if (!node) {
    // Create the node
    node = this._servicePane.createNode();
    node.id = id;
  }

  // Refresh the information just in case it is supposed to change
  node.contractid = CONTRACTID;
  node.setAttributeNS(DEVICESP_NS, "device-id", aDevice.id);
  node.setAttributeNS(DEVICESP_NS, "deviceNodeType", "device");
  node.setAttributeNS(SP, "Weight", DEVICE_NODE_WEIGHT);
  if (aEjectable) {
    node.setAttribute("ejectable", "true");
    let listener = new _deviceNodeEventListener(aDevice);
    node.addEventListener(listener);
  }
  node.editable = false;
  node.className = "device";

  try {
    var iconUri = aDevice.properties.iconUri;
    if (iconUri) {
      var spec = iconUri.spec;
      if (iconUri.schemeIs("file") && /\.ico$/i(spec)) {
        // for *.ico, try to get the small icon
        spec = "moz-icon://" + spec + "?size=16";
      }
      node.image = spec;
    }
  } catch(ex) {
    /* we do not care if setting the icon fails */
  }

  // Add the node
  if (!node.parentNode) {
    this.log("\tNo parentNode, appending to devices group node");
    devicesNode.appendChild(node);
  }

  this._device = aDevice;
  return node;
}

sbDeviceServicePane.prototype.createLibraryNodeForDevice =
function sbDeviceServicePane_createLibraryNodeForDevice(aDevice, aLibrary) {
  this.log("Creating library nodes for device " + aDevice.id);
  var deviceNode = this.getNodeForDevice(aDevice);

  // Create the library nodes and move them underneath the device node
  this._libraryServicePane.createNodeForLibrary(aLibrary);
  this._moveLibraryNodes(aLibrary);

  return this._libraryServicePane.getNodeForLibraryResource(aLibrary);
}

sbDeviceServicePane.prototype.getNodeForDevice =
function sbDeviceServicePane_getNodeForDevice(aDevice) {
  // Get the Node.
  var id = this._deviceURN2(aDevice);
  return this._servicePane.getNode(id);
}

sbDeviceServicePane.prototype.setFillDefaultContextMenu =
function sbDeviceServicePane_setFillDefaultContxtMenu(aNode,
                                                      aEnabled) {
  if (aEnabled) {
    aNode.setAttributeNS(DEVICESP_NS, "fillDefaultContextMenu", "true");
  } else {
    aNode.setAttributeNS(DEVICESP_NS, "fillDefaultContextMenu", "false");
  }
}

sbDeviceServicePane.prototype.insertChildByName =
function sbDeviceServicePane_insertChildByName(aDevice, aChild) {
  var lastNode = null;
  var deviceNode = this.getNodeForDevice(aDevice);
  for (let node = deviceNode.firstChild; node; node = node.nextSibling) {
    var listType = node.getAttributeNS(LSP, "ListType");
    // Ignore library node.
    if (listType == "library") {
      continue;
    }

    // Find the node to insert before. Ignore case.
    var childname = aChild.name ? aChild.name.toLowerCase() : aChild.name;
    var nodename = node.name ? node.name.toLowerCase() : node.name;
    if (childname < nodename) {
      lastNode = node;
      break;
    }
  }

  // Insert before the node found, or insert at the end if lastNode is null.
  deviceNode.insertBefore(aChild, lastNode);
}

////////////////////////////////////
// sbIServicePaneMutationListener //
////////////////////////////////////
function _deviceNodeEventListener(aDevice) {
  this._device = aDevice;
}
_deviceNodeEventListener.prototype = {
  handleEvent: function sbDeviceServicePane_handleEvent(aEventName) {
    switch (aEventName) {
      case "eject":
        try {
          this._device.eject();
        } catch (e) {
          dump("Exception in sbDeviceServicePane event listener: " + e + "\n");
        }
        break;
    }
  }
}

////////////////////////////////////
// sbIServicePaneMutationListener //
////////////////////////////////////
sbDeviceServicePane.prototype.attrModified =
function sbDeviceServicePane_attrModified(aNode, aAttrName, aNamespace,
                                          aOldVal, aNewVal) {
  // Ensure that the parent device's node reflects any changes.
  this._updateParentDeviceNode();
};

sbDeviceServicePane.prototype.nodeInserted =
function sbDeviceServicePane_nodeInserted(aNode, aParent, aInsertBefore) {
  // Ensure that the parent device's node reflects any changes.
  this._updateParentDeviceNode();
};

sbDeviceServicePane.prototype.nodeRemoved =
function sbDeviceServicePane_nodeRemoved(aNode, aParent) {
  // Ensure that the parent device's node reflects any changes.
  this._updateParentDeviceNode();
};

/////////////////////
// Private Methods //
/////////////////////

/**
 *
 */
sbDeviceServicePane.prototype._updateParentDeviceNode =
function sbDeviceServicePane__updateParentDeviceNode() {
  try {
    // Check to see if the devices group should be hidden or not
    let hidden = true;
    let devicesNode = this._servicePane.getNode("SB:Devices");
    if (!devicesNode) {
      // there's no devices to look at, we don't care
      return;
    }
    for (let node = devicesNode.firstChild; node; node = node.nextSibling) {
      if (node && !node.hidden) {
        hidden = false;
        break;
      }
    }

    if (devicesNode.hidden != hidden) {
      this.log("Hiding devices node since all children are gone or hidden");
      devicesNode.hidden = hidden;
    }
  }
  catch (e) {
    this.log("Execption updating device node: " + e);
  }
};

/**
 * Get a service pane identifier for the given device
 */
sbDeviceServicePane.prototype._deviceURN =
function sbDeviceServicePane__deviceURN(aDevice, aDeviceIdentifier) {
  return URN_PREFIX_DEVICE + aDevice.deviceCategory + ":" + aDeviceIdentifier;
}

/**
 * Ensure the "Devices" service pane group exists (where all devices live)
 */
sbDeviceServicePane.prototype._ensureDevicesGroupExists =
function sbDeviceServicePane__ensureDevicesGroupExists() {
  this.log("ensureDevicesGroupExists");
  let fnode = this._servicePane.getNode("SB:Devices");
  if (!fnode) {
    // make sure it exists
    fnode = this._servicePane.createNode();
    fnode.id = "SB:Devices";
    fnode.name = "&servicesource.devices";
    this._addClassNames(fnode, ["folder", this._makeCSSProperty(fnode.name)]);
    fnode.contractid = CONTRACTID;
    // XXXstevel need to set fnode.dndAcceptIn?
    fnode.editable = false;
    fnode.setAttributeNS(SP, 'Weight', -2);
    this._servicePane.root.appendChild(fnode);
    fnode.addMutationListener(this);
    this._deviceGroupNode = fnode;
    this.log("\tDevices group created");
  }

  return fnode;
}

sbDeviceServicePane.prototype._addClassNames =
function sbDeviceServicePane__addClassNames(aNode, aList) {
  let className = aNode.className || "";
  let existing = {};
  for each (let name in className.split(" "))
    existing[name] = true;

  for each (let name in aList)
    if (!existing.hasOwnProperty(name))
      className += (className ? " " : "") + name;

  aNode.className = className;
}


sbDeviceServicePane.prototype._makeCSSProperty =
function sbDeviceServicePane__makeCSSProperty(aString) {
  if ( aString[0] == "&" ) {
    aString = aString.substr(1, aString.length);
    aString = aString.replace(/\./g, "-");
  }
  return aString;
}


sbDeviceServicePane.prototype._deviceURN2 =
function sbDeviceServicePane__deviceURN2(aDevice) {
  var id = "" + aDevice.id;
  
  if(id.charAt(0) == "{" &&
     id.charAt(-1) == "}") {
    id = id.substring(1, -1);
  }
  
  
  return URN_PREFIX_DEVICE + id;
}


/**
 * Move all library nodes for the library specified by aLibrary to the device
 * node.
 *
 * \param aLibrary              Library for which to move nodes.
 */
sbDeviceServicePane.prototype._moveLibraryNodes =
function sbDeviceServicePane__moveLibraryNodes(aLibrary) {
  var nodeList = this._libraryServicePane.getNodesForLibraryResource(aLibrary);
  nodeList = ArrayConverter.JSArray(nodeList);
  for each (var node in nodeList) {
    node.QueryInterface(Ci.sbIServicePaneNode);
    this._moveLibraryResourceNode(node);
  }
}


/**
 * Get the index out of the type array based on the node class name.
 *
 * \param aNode              Library resource node.
 */
sbDeviceServicePane.prototype._getNodeIndexFromClassName =
function sbDeviceServicePane__getNodeIndexFromClassName(aNode) {
  const K_TYPES = ["audio", "video", "podcast"];
  var types = aNode.className.split(/\s/);
  var index = -1;
  for (let i = 0; i < types.length; ++i) {
    if ((index = K_TYPES.indexOf(types[i])) > -1)
      break;
  }

  return index;
}


/**
 * Move the library resource node specified by aNode to the device node.  Do
 * nothing if node is not a device library resource node.
 *
 * \param aNode                 Library resource node to move.
 */
sbDeviceServicePane.prototype._moveLibraryResourceNode =
function sbDeviceServicePane__moveLibraryResourceNode(aNode) {
  var resource = this._libraryServicePane.getLibraryResourceForNode(aNode);
  if (!resource) {
    return;
  }
  if (!(resource instanceof Ci.sbIMediaList)) {
    // not a media list, we don't care
    return;
  }
  var device = this._deviceMgr.getDeviceForItem(resource);
  if (!device) {
    // not a device playlist
    return;
  }

  // Only move nodes that the device supports
  var functions = device.capabilities.getSupportedFunctionTypes({});
  const CAPS_MAP = {
    "audio": Ci.sbIDeviceCapabilities.FUNCTION_AUDIO_PLAYBACK,
    "video": Ci.sbIDeviceCapabilities.FUNCTION_VIDEO_PLAYBACK
  };

  var props = aNode.className.split(/\s/);
  props = props.filter(function(val) val in CAPS_MAP);
  // there should only be one anyway...
  // assert(props.length < 2);
  if (functions.indexOf(CAPS_MAP[props[0]]) < 0) {
    this.log("Not moving library node " + aNode.id +
             " to device node, capability not supported");
    return;
  }

  // Set up the device library node info.
  aNode.setAttributeNS(DEVICESP_NS, "device-id", device.id);
  aNode.setAttributeNS(DEVICESP_NS, "deviceNodeType", "library");

  var deviceNode = this.getNodeForDevice(device);
  if (deviceNode && aNode.parentNode != deviceNode) {
    var index1 = this._getNodeIndexFromClassName(aNode);
    if (index1 == -1) {
      dump("_moveLibraryResourceNode: Not in the node array??!!\n");
      deviceNode.appendChild(aNode);
      return;
    }

    var child = deviceNode.firstChild;
    while (child) {
      var index2 = this._getNodeIndexFromClassName(child);
      if (index2 == -1 || index1 < index2)
        break;

      child = child.nextSibling;
    }
    deviceNode.insertBefore(aNode, child);
  }
}

///////////
// XPCOM //
///////////

var NSGetModule = XPCOMUtils.generateNSGetModule([sbDeviceServicePane]);
