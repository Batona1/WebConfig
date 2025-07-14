//ArduinoJson V7 compatible
#include "WebConfig.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>

WebConfig::WebConfig() {
}

void WebConfig::setDescription(String json) {
  _json = json;
  parseJson();
}

void WebConfig::setFileHandler(THandlerFunction fn) {
  _fileHandler = fn;
}

void WebConfig::setButtonHandler(THandlerFunction fn) {
  _btnHandler = fn;
}

void WebConfig::handleFormRequest(WebServer *server) {
  if (server->hasArg("SAVE")) {
    for (int i = 0; i < _count; i++) {
      String val = server->arg(_params[i].name);
      val.trim();
      if (val.length() > 0) {
        if (_params[i].type == INPUTTEXT || _params[i].type == INPUTPASSWORD || _params[i].type == INPUTEMAIL || _params[i].type == INPUTDATE || _params[i].type == INPUTTIME) {
          strlcpy(_params[i].value, val.c_str(), sizeof(_params[i].value));
        } else if (_params[i].type == INPUTNUMBER || _params[i].type == INPUTFLOAT) {
          strlcpy(_params[i].value, val.c_str(), sizeof(_params[i].value));
        } else if (_params[i].type == INPUTCHECKBOX) {
          strlcpy(_params[i].value, "1", sizeof(_params[i].value));
        } else if (_params[i].type == INPUTRADIO) {
          strlcpy(_params[i].value, val.c_str(), sizeof(_params[i].value));
        }
      } else {
        if (_params[i].type == INPUTCHECKBOX) {
          strlcpy(_params[i].value, "0", sizeof(_params[i].value));
        }
      }
    }
    saveConfig();
  }

  if (server->hasArg("BUTTON")) {
    if (_btnHandler) {
      _btnHandler();
    }
  }

  String s = "<html><head><title>WebConfig</title></head><body>";
  s += "<h1>WebConfig</h1>";
  s += "<form method='POST' action=''>";
  s += "<table>";

  for (int i = 0; i < _count; i++) {
    s += "<tr><td>" + String(_params[i].label) + "</td><td>";
    if (_params[i].type == INPUTTEXT || _params[i].type == INPUTPASSWORD || _params[i].type == INPUTEMAIL || _params[i].type == INPUTDATE || _params[i].type == INPUTTIME || _params[i].type == INPUTNUMBER || _params[i].type == INPUTFLOAT) {
      s += "<input type=';
      if (_params[i].type == INPUTTEXT) s += "text";
      if (_params[i].type == INPUTPASSWORD) s += "password";
      if (_params[i].type == INPUTEMAIL) s += "email";
      if (_params[i].type == INPUTDATE) s += "date";
      if (_params[i].type == INPUTTIME) s += "time";
      if (_params[i].type == INPUTNUMBER) s += "number";
      if (_params[i].type == INPUTFLOAT) s += "number' step='any";
      s += "' name='" + String(_params[i].name) + "' value='" + String(_params[i].value) + "'>";
    } else if (_params[i].type == INPUTCHECKBOX) {
      s += "<input type='checkbox' name='" + String(_params[i].name) + "' value='1'";
      if (strcmp(_params[i].value, "1") == 0) {
        s += " checked";
      }
      s += ">";
    } else if (_params[i].type == INPUTRADIO) {
      for (int j = 0; j < _params[i].options_count; j++) {
        s += "<input type='radio' name='" + String(_params[i].name) + "' value='" + String(_params[i].options[j].value) + "'";
        if (strcmp(_params[i].value, _params[i].options[j].value) == 0) {
          s += " checked";
        }
        s += "> " + String(_params[i].options[j].label) + "<br>";
      }
    }
    s += "</td></tr>";
  }

  s += "</table>";
  s += "<br><br>";
  s += "<input type='submit' name='SAVE' value='Save'>";
  s += "</form>";

  if (_fileHandler) {
    s += "<br><br>";
    s += "<h2>File Upload</h2>";
    s += "<form method='POST' action='' enctype='multipart/form-data'>";
    s += "<input type='file' name='upload'>";
    s += "<input type='submit' value='Upload'>";
    s += "</form>";
  }

  if (_btnHandler) {
    s += "<br><br>";
    s += "<form method='POST' action=''>";
    s += "<input type='submit' name='BUTTON' value='Action'>";
    s += "</form>";
  }

  s += "</body></html>";
  server->send(200, "text/html", s);
}

void WebConfig::parseJson() {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, _json);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  JsonArray array = doc.as<JsonArray>();
  _count = array.size();
  _params = new t_param[_count];
  int i = 0;
  for (JsonVariant v : array) {
    JsonObject obj = v.as<JsonObject>();
    strlcpy(_params[i].name, obj["name"], sizeof(_params[i].name));
    strlcpy(_params[i].label, obj["label"], sizeof(_params[i].label));
    _params[i].type = obj["type"];
    if (obj.containsKey("default")) {
      strlcpy(_params[i].value, obj["default"], sizeof(_params[i].value));
    } else {
      _params[i].value[0] = '\0';
    }
    if (_params[i].type == INPUTRADIO) {
      JsonArray options = obj["options"].as<JsonArray>();
      _params[i].options_count = options.size();
      _params[i].options = new t_option[_params[i].options_count];
      int j = 0;
      for (JsonVariant o : options) {
        JsonObject option = o.as<JsonObject>();
        strlcpy(_params[i].options[j].value, option["v"], sizeof(_params[i].options[j].value));
        strlcpy(_params[i].options[j].label, option["l"], sizeof(_params[i].options[j].label));
        j++;
      }
    }
    i++;
  }
}

void WebConfig::saveConfig() {
  JsonDocument doc;
  for (int i = 0; i < _count; i++) {
    doc[_params[i].name] = _params[i].value;
  }
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  serializeJson(doc, configFile);
  configFile.close();
}

void WebConfig::readConfig() {
  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, configFile);
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }
        for (int i = 0; i < _count; i++) {
          if (doc.containsKey(_params[i].name)) {
            strlcpy(_params[i].value, doc[_params[i].name], sizeof(_params[i].value));
          }
        }
        configFile.close();
      }
    }
  }
}

String WebConfig::getString(String name) {
  for (int i = 0; i < _count; i++) {
    if (strcmp(_params[i].name, name.c_str()) == 0) {
      return String(_params[i].value);
    }
  }
  return "";
}

int WebConfig::getInt(String name) {
  for (int i = 0; i < _count; i++) {
    if (strcmp(_params[i].name, name.c_str()) == 0) {
      return atoi(_params[i].value);
    }
  }
  return 0;
}

float WebConfig::getFloat(String name) {
  for (int i = 0; i < _count; i++) {
    if (strcmp(_params[i].name, name.c_str()) == 0) {
      return atof(_params[i].value);
    }
  }
  return 0;
}

bool WebConfig::getBool(String name) {
  for (int i = 0; i < _count; i++) {
    if (strcmp(_params[i].name, name.c_str()) == 0) {
      return (strcmp(_params[i].value, "1") == 0);
    }
  }
  return false;
}
