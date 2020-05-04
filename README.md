# LilyGO-TTGO-HiGrow
## TTGO-HIGrow MQTT interface for Homeassistant

![T-Higrow](\\ds918\docker\GitHub\LilyGO-TTGO-HiGrow\images\T-Higrow.jpg)



### Getting started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

What things you need to install the software and how to install them

1. LilyGo T-Higrow V1.1 (available from AliExpress)

2. Windows 10, with installed Arduino EDI (my version 1.8.11)

3. USB Cable with USB-C to attatch to the LilyGo

4. MQTT server (I am running on a Synology NAS in docker)
   If you have a Synology NAS, I can recommend to follow BeardedTinker on YouTube, he makes a very intuitive explanation how to setup the whole environment on Synology.   

   [](https://https://www.youtube.com/channel/UCuqokNoK8ZFNQdXxvlE129g)

### Installing

A step by step that tell you how to get a development/production environment up and running

1. Setup the Arduino EDI
2. Download the .ino file
3. Edit the .ino file where it is marked that you should edit
4. Add the following to your configuration.yaml:

```yaml
#Northvindue
  - platform: mqtt
    name: North_Window_Battery_8
    state_topic: "NorthWindow/North_8"
    value_template: "{{ value_json.bat.bat }}"
    unit_of_measurement: '%'
    icon: mdi:battery
  - platform: mqtt    
    name: North_Window_salt_8
    state_topic: "NorthWindow/North_8"
    value_template: "{{ value_json.salt.salt }}"
    unit_of_measurement: 'q' 
    icon: mdi:bottle-tonic
  - platform: mqtt
    name: North_Window_Time_8
    state_topic: "NorthWindow/North_8"
    value_template: "{{ value_json.time.time }}"
    icon: mdi:clock
  - platform: mqtt
    name: North_Window_Date_8
    state_topic: "NorthWindow/North_8"
    value_template: "{{ value_json.date.date }}"
    icon: mdi:calendar    
  - platform: mqtt
    name: North_Window_Soil_8
    state_topic: "NorthWindow/North_8"
    value_template: "{{ value_json.soil.soil }}"
    unit_of_measurement: '%' 
    icon: mdi:water-percent
  - platform: mqtt
    name: North_Window_no_8
    state_topic: "NorthWindow/North_8"
    value_template: "{{ value_json.NorthWindow.NorthWindow }}"
    icon: mdi:numeric-8
```

​		and the data will be available in Homeassistant.
​        Create a "Blink" card:

![Nord-Window](\\ds918\docker\GitHub\LilyGO-TTGO-HiGrow\images\Nord-Window.JPG)

You then just use this .ino as a master, and create a copy for each LilyGo T-Higrow V1.1 you have.

### Running

The LilyGo T-Higrow V1.1, will wake up every (in this case) hour and report status to the MQTT server, and at the same time it will be updated in Homeassistant. It will run for weeks on a 3.7V Lithium battery.

You can set the wakeup time as you want. Lower time higher battery consumption, Higher time lower battery consumption.

All censers from the LilyGo T-Higrow V1.1 are updated, so you can easy include them in Homeassistant if you should wish so.

### Alarm for low soil humidity

You can make the Homeassistant give you an alarm for low Soil Humidity, you will have to add the following to your automations.yaml.

```yaml
- alias: 'nord_window_soil_8'
  trigger:
    platform: numeric_state
    entity_id: sensor.north_window_soil_8
    below: 30
    for:
      minutes: 30
  action:
  - service: notify.mobile_app_xxxxxxx_iphone
    data:
      message: 'North window plant 8, needs watering'

```

## Deployment

See instructions under **Prerequisites**

## Versioning

1.0.0

## Authors

* **Per Rose** 

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* **Xinyuan-LilyGO / TTGO-HiGrow**  [https://github.com/Xinyuan-LilyGO/TTGO-HiGrow]( https://github.com/Xinyuan-LilyGO/TTGO-HiGrow) 

* I changed the above from a WEB server to MQTT, with integration into Homeassistant




### 3D printed case for the sensor

I am working on a case to fit the sensor into, including a 800mA lithium battery.

![Case](%5C%5Cds918%5Cdocker%5CGitHub%5CLilyGO-TTGO-HiGrow%5Cimages%5CCase.jpg)

![Case-detail](%5C%5Cds918%5Cdocker%5CGitHub%5CLilyGO-TTGO-HiGrow%5Cimages%5CCase-detail.jpg)

I have included the original file for the cases in FreeCad format.