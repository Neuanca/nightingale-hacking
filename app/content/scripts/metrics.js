/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END NIGHTINGALE GPL
//
 */

// Metrics

// (kinda simplified now that the functionality is in the service)

var gMetrics = Components.classes["@getnightingale.com/Nightingale/Metrics;1"]
                  .createInstance(Components.interfaces.sbIMetrics); 

function metrics_add( category, unique_id, extra, intvalue )
{
  gMetrics.metricsAdd( category, unique_id, extra, intvalue );
}

function metrics_inc( category, unique_id, extra )
{
  gMetrics.metricsInc( category, unique_id, extra );
}

