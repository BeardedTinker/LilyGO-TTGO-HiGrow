# LilyGO-TTGO-HiGrow
## TTGO-HIGrow MQTT interface for Homeassistant

![T-Higrow](https://github.com/pesor/LilyGO-TTGO-HiGrow/blob/master/images/T-Higrow.jpg)



### Getting started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

What things you need to install the software and how to install them

1. LilyGo T-Higrow V1.1 (available from AliExpress)  (make sure it is V1.1 19-8-22 or later)

2. Windows 10, with installed Arduino EDI (my version 1.8.12)

3. USB Cable with USB-C to attatch to the LilyGo

4. MQTT server (I am running on a Synology NAS in docker)
   If you have a Synology NAS, I can recommend to follow [BeardedTinker](https://www.youtube.com/channel/UCuqokNoK8ZFNQdXxvlE129g) on YouTube, he makes a very intuitive explanation how to setup the whole environment on Synology.   

   [](https://https://www.youtube.com/channel/UCuqokNoK8ZFNQdXxvlE129g)

### Installing

A step by step that tell you how to get a development/production environment up and running

1. Setup the Arduino EDI
2. Download the .ino file
3. Edit the .ino file where it is marked that you should edit
4. Add the following to your configuration.yaml ( change name and state_topic accordint to your needs):

```yaml
#Greenhouse
  - platform: mqtt
    name: Drivhus_Padron_Battery_1
    state_topic: "Drivhus/Padron_1"
    value_template: "{{ value_json.bat.bat }}"
    unit_of_measurement: '%'
    icon: mdi:battery
  - platform: mqtt    
    name: Drivhus_Padron_salt_1
    state_topic: "Drivhus/Padron_1"
    value_template: "{{ value_json.salt.salt }}"
    unit_of_measurement: 'q' 
    icon: mdi:bottle-tonic
  - platform: mqtt
    name: Drivhus_Padron_Time_1
    state_topic: "Drivhus/Padron_1"
    value_template: "{{ value_json.time.time }}"
    icon: mdi:clock
  - platform: mqtt
    name: Drivhus_Padron_Date_1
    state_topic: "Drivhus/Padron_1"
    value_template: "{{ value_json.date.date }}"
    icon: mdi:calendar    
  - platform: mqtt
    name: Drivhus_Padron_Soil_1
    state_topic: "Drivhus/Padron_1"
    value_template: "{{ value_json.soil.soil }}"
    unit_of_measurement: '%' 
    icon: mdi:water-percent
  - platform: mqtt
    name: Drivhus_Padron_Temp_1
    state_topic: "Drivhus/Padron_1"
    value_template: "{{ value_json.temp.temp }}"
    unit_of_measurement: 'C' 
    icon: mdi:home-thermometer
  - platform: mqtt
    name: Drivhus_Padron_Humid_1
    state_topic: "Drivhus/Padron_1"
    value_template: "{{ value_json.humid.humid }}"
    unit_of_measurement: '%' 
    icon: mdi:water-percent
  - platform: mqtt
    name: Drivhus_Padron_rel_1
    state_topic: "Drivhus/Padron_1"
    value_template: "{{ value_json.rel.rel }}"
    icon: mdi:alert-decagram
  - platform: mqtt
    name: Drivhus_Padron_no_1
    state_topic: "Drivhus/Padron_1"
    value_template: "{{ value_json.Drivhus.Drivhus }}"
    icon: mdi:numeric-1

```

​		and the data will be available in Homeassistant.
​        Create a "Blink" card:

![Green-house](https://github.com/pesor/LilyGO-TTGO-HiGrow/blob/master/images/Green-house.jpg)

You then just use this .ino as a master, and create a copy for each LilyGo T-Higrow V1.1 you have.

Follow the code, and calibrate your Soil sensor, first take the value when unit is on the table, then take the value when unit is placed in water (up to the electronics), these two readings you place in the right places of the map statement.

### Running

The LilyGo T-Higrow V1.1, will wake up every (in this case) hour and report status to the MQTT server, and at the same time it will be updated in Homeassistant. It will run for approx. 2 months on a 3.7V Lithium battery.

I have had a couple of units, which did not last that long. It turns out that one were using 5mA, and the other 14.6 mA when in sleep mode. They are not suitable for battery, and I have no clue why they consume this amount of power. The average consumption for my other boards are around 0.250 mA, which is according to factory specifications.

You can set the wakeup time as you want. Lower time higher battery consumption, Higher time lower battery consumption.

All sensors from the LilyGo T-Higrow V1.1 are updated, so you can easy include them in Homeassistant if you should wish so.

### Alarm for low soil humidity

You can make the Homeassistant give you an alarm for low Soil Humidity, you will have to add the following to your automations.yaml. (example)

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

1.5 Major release update, introducing logging, change to PubSubClient, minor error corrections, and tuning related to battery usage.

## Authors

* **Per Rose** 

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* **Xinyuan-LilyGO / TTGO-HiGrow**  [https://github.com/Xinyuan-LilyGO/TTGO-HiGrow]( https://github.com/Xinyuan-LilyGO/TTGO-HiGrow) 

* I changed the above from a WEB server to MQTT, with integration into Homeassistant




### 3D printed case for the sensor

I am working on a case to fit the sensor into, including a 800mA lithium battery.

![Case](https://github.com/pesor/LilyGO-TTGO-HiGrow/blob/master/images/Case.jpg)

![Case-detail](https://github.com/pesor/LilyGO-TTGO-HiGrow/blob/master/images/Case-detail.jpg)

I have included the original file for the cases in FreeCad format.
