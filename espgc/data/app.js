'use strict';
/* global nanoajax */
// using microlibraries
// https://github.com/finom/balalaika
// https://github.com/yanatan16/nanoajax

$.ajax = nanoajax.ajax;

$.fn.hasClass = function(className) {
  return !!this[0] && this[0].classList.contains(className);
};

$.fn.addClass = function(className) {
  this.forEach(function(item) {
    var classList = item.classList;
    classList.add.apply(classList, className.split(/\s/));
  });
  return this;
};

$.fn.removeClass = function(className) {
  this.forEach(function(item) {
    var classList = item.classList;
    classList.remove.apply(classList, className.split(/\s/));
  });
  return this;
};

$.fn.toggleClass = function(className, b) {
  this.forEach(function(item) {
    var classList = item.classList;
    if (typeof b !== 'boolean') {
      b = !classList.contains(className);
    }
    classList[b ? 'add' : 'remove'].apply(classList, className.split(/\s/));
  });
  return this;
};

$.fn.val = function(v) {
  if (arguments.length > 0) {
    this.forEach(function(item) {
      item.value = v;
    });
    return this;
  } else {
    // or comma-separated values
    return this ? this[0].value : null;
  }
};

$.fn.prop = function(p, v) {
  if (arguments.length > 1) {
    this.forEach(function(item) {
      item[p] = v;
    });
    return this;
  } else {
    //this.forEach(function (item) {
    //    return item[p];
    //});
    return this ? this[0][p] : null;
  }
};

$.fn.hide = function() {
  this.forEach(function(item) {
    item._data = item._data || {};
    if (typeof (item._data.oldDisplay) === 'undefined') {
      item._data.oldDisplay = item.style.display;
      item.style.display = 'none';
    }
  });
  return this;
};

$.fn.show = function() {
  this.forEach(function(item) {
    item._data = item._data || {};
    if (typeof (item._data.oldDisplay) !== 'undefined') {
      item.style.display = item._data.oldDisplay;
      delete item._data.oldDisplay;
    }
  });
  return this;
};

$.fn.html = function(v) {
  this.forEach(function(item) {
    item.innerHTML = v;
  });
  return this;
};

//debug (function app() {

var networkPollTime = 5 * 60 * 1000,
  statusTime = 5000,
  restartTime = 10000,
  selectedTab = '',
  connected = false,
  maxModules = 99,
  maxConnectors = 99;

function checkStatic() {
  if ($('[name=dhcp]').is(':checked')) {
    disableStatic(true);
  } else {
    disableStatic(false);
  }
  validateStatic();
}

function disableStatic(b) {
  $('[name=staticIp]').prop('disabled', b);
  $('[name=gatewayIp]').prop('disabled', b);
  $('[name=subnetMask]').prop('disabled', b);
}

function validateStatic() {
  if ($('[name=dhcp]').is(':checked')) {
    $('.staticIpError').hide();
    $('.gatewayIpError').hide();
    $('.subnetMaskError').hide();
    return;
  }
  if (validateIpAddress($('[name=staticIp]').val())) {
    $('.staticIpError').hide();
  } else {
    $('.staticIpError').html('Bad Format!');
    $('.staticIpError').show();
  }
  if (validateIpAddress($('[name=gatewayIp]').val())) {
    $('.gatewayIpError').hide();
  } else {
    $('.gatewayIpError').html('Bad Format!');
    $('.gatewayIpError').show();
  }
  if (validateIpAddress($('[name=subnetMask]').val())) {
    $('.subnetMaskError').hide();
  } else {
    $('.subnetMaskError').html('Bad Format!');
    $('.subnetMaskError').show();
  }
}

function setFields(data) {
  var i, html, modconn, tokens;

  console.log('setFields', data);

  $('#title').html(data.deviceName);
  $('#deviceName').html(data.deviceName);
  $('#tagline').html(data.tagline);
  $('#deviceStatus').html('Last boot: ' + data.lastBoot + '...Locked: ' + (data.locked ? 'yes' : 'no'));

  $('[name=version]').val(data.version);
  $('[name=updated]').val(new Date(data.updated * 1000).toLocaleString());
  $('[name=platform]').val(data.platform);
  $('[name=deviceName]').val(data.deviceName);
  $('[name=tagline]').val(data.tagline);
  $('[name=MAC]').val(data.mac);
  $('[name=wirelessMode]').val(data.wirelessMode);
  $('[name=hostname]').val(data.hostname);
  $('[name=ssid]').val(data.ssid);
  $('[name=passphrase]').val(data.passphrase);
  if (data.locked) {
    $('[name=passphrase]').prop('disabled', true);
  }
  $('[name=dhcp]').prop('checked', data.dhcp);
  checkStatic();
  if (data.staticIp) {
    $('[name=staticIp]').val(data.staticIp);
  } else {
    $('[name=staticIp]').val('');
  }
  if (data.gatewayIp) {
    $('[name=gatewayIp]').val(data.gatewayIp);
  } else {
    $('[name=gatewayIp]').val('');
  }
  if (data.subnetMask) {
    $('[name=subnetMask]').val(data.subnetMask);
  } else {
    $('[name=subnetMask]').val('');
  }
  $('[name=tcp]').prop('checked', data.tcp);
  $('[name=discovery]').prop('checked', data.discovery);

  html = '';
  $('#modules').html(html);
  for (i = 0; i < maxModules; i++) {
    modconn = data['module_' + i];
    if (modconn) {
      tokens = modconn.split(',');
      html += '<div>';
      html += '<span class="moduleNum">' + i + '</span>';
      html += '<span class="moduleType">' + tokens[0] + '</span>';
      //html += '<span class="moduleFPin">' + tokens[1] + '</span>';
      //html += '<span class="moduleSPin">' + tokens[2] + '</span>';
      html += '</div>';
    }
  }
  $('#modules').html(html);

  html = '';
  $('#connectors').html(html);
  for (i = 0; i < maxConnectors; i++) {
    modconn = data['connector_' + i];
    if (modconn) {
      tokens = modconn.split(',');
      html += '<div>';
      html += '<span class="connectorNum">' + i + '</span>';
      html += '<span class="connectorType">' + tokens[0] + '</span>';
      html += '<span class="connectorModule">' + tokens[1] + '</span>';
      html += '<span class="connectorAddress">' + tokens[2] + '</span>';
      html += '<span class="connectorFPin">' + tokens[3] + '</span>';
      html += '<span class="connectorSPin">' + tokens[4] + '</span>';
      html += '</div>';
    }
  }
  $('#connectors').html(html);

}

// http://www.w3resource.com/javascript/form/ip-address-validation.php
function validateIpAddress(ipAddress) {
  if (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(ipAddress)) {
    return (true);
  }
  return (false);
}

function loadConfig(quiet) {
  if (!quiet) {
    $('.status').html('Retrieving settings...');
    $('.status').addClass('ok');
    $('.status').show();
  }
  $.ajax({ url: '/api/config' }, function(code, res, xhr) {

    console.log('/api/config');
    console.log(res);
    console.log('----------------');
    res = JSON.parse(res);
    if ((code === 200) && (!res.err)) {
      if (!connected) {
        firstConnect();
      }
      setFields(res.res);
      if (!quiet) {
        $('.status').hide();
        $('.status').removeClass('ok');
      }
    } else {
      $('.status').html('Error retrieving settings!');
      $('.status').addClass('error');
      $('.status').show();
      setTimeout(function() {
        $('.status').hide();
        $('.status').removeClass('error');
      }, statusTime);
    }
    loadNetworks();
  });
}

function saveConfig() {
  var body = '';

  body += 'updated=' + Math.floor(Date.now() / 1000);

  body += '&devicename=' + $('[name=deviceName]').val();
  body += '&tagline=' + $('[name=tagline]').val();
  body += '&platform=' + $('[name=platform]').val();
  body += '&hostname=' + $('[name=hostname]').val();
  body += '&wirelessMode=' + $('[name=wirelessMode]').val();                  // 0/1
  body += '&ssid=' + $('[name=ssid]').val();
  body += '&passphrase=' + $('[name=passphrase]').val();
  body += '&dhcp=' + ($('[name=dhcp]').prop('checked') ? 1 : 0);          // 0/1
  body += '&staticIp=' + $('[name=staticIp]').val();
  body += '&gatewayIp=' + $('[name=gatewayIp]').val();
  body += '&subnetMask=' + $('[name=subnetMask]').val();
  body += '&tcp=' + ($('[name=tcp]').prop('checked') ? 1 : 0);          // 0/1
  body += '&discovery=' + ($('[name=discovery]').prop('checked') ? 1 : 0);          // 0/1
  console.log(body);

  $.ajax({ url: '/api/config', method: 'POST', body: body }, function(code, res, xhr) {

    console.log('POST /api/config');
    console.log(body);
    console.log('----------------');
    console.log(code);
    console.log(res);
    console.log('----------------');
    res = JSON.parse(res);
    if ((code === 200) && (!res.err)) {
      // redisplay just to make sure...
      setFields(res.res);
      $('.status').html('Settings saved.');
      $('.status').addClass('ok');
      $('.status').show();
      loadConfigFile();
    } else {
      $('.status').html('Error saving settings!');
      $('.status').addClass('error');
      $('.status').show();
    }
    setTimeout(function() {
      $('.status').hide();
      $('.status').removeClass('ok');
      $('.status').removeClass('error');
    }, statusTime);
  });

}

function loadNetworks() {
  var data = {};
  var i;
  var html = '';
  var n;

  $.ajax({ url: '/api/networks' }, function(code, res, xhr) {

    console.log('/api/networks');
    console.log(code);
    console.log(res);
    console.log('----------------');
    res = JSON.parse(res);
    if ((code === 200) && (!res.err)) {
      try {
        data = JSON.parse(res.res);
      } catch (e) {
        console.log('Bad parse from /api/networks');
        console.log(res);
        data = { networks: [] };
      }
      html = '<h3>Available Networks <span class="updated">Updated: ' + new Date().toLocaleString() + '</span></h3><div class="list">';
      if (data.networks) {
        for (i = 0; i < data.networks.length; i++) {
          n = data.networks[i];
          html += 'Chan: ' + n.channel + ' BSSID: ' + n.bssid + ' Enc: ' + n.encryption + ' RSSI: ' + n.rssi + ' SSID: ' + n.ssid + '\n';
        }
        html += '</div>';
        $('.networks').html(html);
      }
      //$('.status').hide();
      //$('.status').removeClass('ok');
    } else {
      $('.networks').html('');
      $('.status').html('Error retrieving available networks!');
      $('.status').addClass('error');
      $('.status').show();
      setTimeout(function() {
        $('.status').hide();
        $('.status').removeClass('error');
      }, statusTime);

    }
  });
}

function loadConfigFile() {
  $.ajax({ url: '/api/configfile' }, function(code, res, xhr) {

    console.log('/api/configfile');
    console.log(code);
    console.log(res);
    console.log('----------------');
    res = JSON.parse(res);
    if ((code === 200) && (!res.err)) {
      console.log(res.res);
      $('#configFileText').val(res.res);
    } else {
      $('.networks').html('');
      $('.status').html('Error retrieving config file!');
      $('.status').addClass('error');
      $('.status').show();
      setTimeout(function() {
        $('.status').hide();
        $('.status').removeClass('error');
      }, statusTime);
    }
  });

}

function saveConfigFile() {
  var body = '', text;

  text = $('#configFileText').val().replace(/\nupdated=.*?\n/, '\nupdated=' + (Math.floor(Date.now() / 1000)) + '\n');
  text = encodeURIComponent(text);
  body += 'data=' + text;
  $.ajax({ url: '/api/configfile', method: 'POST', body: body }, function(code, res, xhr) {

    console.log('POST /api/configfile');
    console.log(body);
    console.log('----------------');
    console.log(code);
    console.log(res);
    console.log('----------------');
    res = JSON.parse(res);
    if ((code === 200) && (!res.err)) {
      $('.status').html('Settings saved.');
      $('.status').addClass('ok');
      $('.status').show();
      // redisplay just to make sure...
      loadConfig(true);
      loadConfigFile();
    } else {
      $('.status').html('Error saving settings!');
      $('.status').addClass('error');
      $('.status').show();
    }
    setTimeout(function() {
      $('.status').hide();
      $('.status').removeClass('ok');
      $('.status').removeClass('error');
    }, statusTime);
  });
}

function selectTab(name) {
  $('.tab').removeClass('selected');
  $('.tabContent').hide();
  $('.tab.tab-' + name).addClass('selected');
  $('.tabContent.tab-' + name).show();
  selectedTab = name;
}

function showSelectedTab() {
  selectTab(selectedTab);
}

function firstConnect() {
  console.log('Got connection.');
  $('#noGots').hide();
  showSelectedTab();
  $('.tab').on('click', function() {
    var i, classes;

    classes = $(this).prop('className').split(' ');
    for (i = 0; i < classes.length; i++) {
      if (classes[i].substr(0, 4) === 'tab-') {
        selectTab(classes[i].substr(4));
      }
    }
  });
}

$(function() {

  selectTab('network');
  console.log('Waiting for connection...');
  $('#noGots').show();
  $('.tabContent').hide();

  $('.status').hide();
  $('.inputError').hide();
  $('input').val();

  loadConfig();
  loadConfigFile();
  setInterval(function() {
    loadNetworks();
  }, networkPollTime);

  $('[name=wirelessMode]').on('change', function(ev) {
    $('[name=wirelessMode]').forEach(function(el) {
      if (el.value === '0') {
        $('[name=dhcp]').forEach(function(el) {
          el.disabled = false;
        });
        if ($('[name=dhcp]').is(':checked')) {
          disableStatic(true);
        } else {
          disableStatic(false);
        }
      } else {
        $('[name=dhcp]').forEach(function(el) {
          el.disabled = true;
        });
        disableStatic(true);
      }
    });
  });

  $('[name=dhcp]').on('change', function() {
    checkStatic();
  });

  $('form').on('submit', function(e) {
    e.preventDefault();
  });

  $('[name=submit]').on('click', function() {
    saveConfig();
  });

  $('[name=refresh]').on('click', function() {
    loadConfig();
  });

  $('[name=hostname]').on('change', function() {
    var v = $('[name=hostname]').val().trim();
    $('[name=hostname]').val(v);
    if ((v.length < 1) || (v.length > 31)) {
      $('.hostnameError').html('1:31 chars!');
      $('.hostnameError').show();
    } else {
      $('.hostnameError').hide();
    }
  });

  $('[name=ssid]').on('change', function() {
    var v = $('[name=ssid]').val().trim();
    $('[name=ssid]').val(v);
    if ((v.length < 1) || (v.length > 63)) {
      $('.ssidError').html('1:63 chars!');
      $('.ssidError').show();
    } else {
      $('.ssidError').hide();
    }
  });

  $('[name=passphrase]').on('change', function() {
    var v = $('[name=passphrase]').val().trim();
    $('[name=passphrase]').val(v);
    if ((v.length < 8) || (v.length > 63)) {
      $('.passphraseError').html('8:63 chars!');
      $('.passphraseError').show();
    } else {
      $('.passphraseError').hide();
    }
  });


  $('[name=staticIp]').on('keyup', function() {
    validateStatic();
  });

  $('[name=gatewayIp]').on('keyup', function() {
    validateStatic();
  });

  $('[name=subnetMask]').on('keyup', function() {
    validateStatic();
  });

  $('[name=tcp]').on('change', function() {
    if ($('[name=tcp]').is(':checked')) {
      $('[name=discovery]').prop('disabled', false);
    } else {
      $('[name=discovery]').prop('disabled', true);
    }
  });

  $('#saveConfigFile').on('click', function() {
    saveConfigFile();
  });

  $('#loadConfigFile').on('click', function() {
    loadConfigFile();
  });

  $('#restart').on('click', function() {
    $.ajax({ url: '/api/admin/restart', method: 'POST', body: '' }, function(code, res, xhr) {

      console.log('POST /api/admin/restart');
      console.log(code);
      console.log(res);
      console.log('----------------');
      res = JSON.parse(res);
      if ((code === 200) && (!res.err)) {
        $('.status').html('Restarting...');
        $('.status').addClass('ok');
        $('.status').show();
        setTimeout(function() {
          window.location.reload(1);
        }, restartTime);
      } else {
        $('.status').html('Error restarting!');
        $('.status').addClass('error');
        $('.status').show();
      }
    });
  });

  $('#compress').on('click', function() {
    var url = '/api/compressir?code=' + encodeURIComponent($('#irRegular').val());

    $('#fudge').val(parseInt($('#fudge').val(), 10) || 0);
    if (parseInt($('#fudge').val(), 10) > 0) {
      url += '&fudge=' + $('#fudge').val();
    }
    $('#irCompressed').val('');

    $.ajax({ url: url }, function(code, res) {

      console.log('/api/compressir');
      console.log(code);
      console.log(res);
      console.log('----------------');
      res = JSON.parse(res);
      if ((code === 200) && (!res.err)) {
        res = JSON.parse(res);
        $('#irCompressed').val(res.code);
      } else {
      }
    });
  });

  $('#decompress').on('click', function() {
    var url = '/api/decompressir?code=' + encodeURIComponent($('#irCompressed').val());
    $('#irRegular').val('');

    $.ajax({ url: url }, function(code, res) {

      console.log('/api/decompressir');
      console.log(code);
      console.log(res);
      console.log('----------------');
      res = JSON.parse(res);
      if ((code === 200) && (!res.err)) {
        res = JSON.parse(res);
        $('#irRegular').val(res.code);
      } else {
      }
    });
  });

});

//})();
