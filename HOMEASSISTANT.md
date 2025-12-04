# Home Assistant Integration Guide for Az3166 Board

## Quick Setup

### Step 1: Find Your Az3166 IP Address
Your board displays its IP address on the OLED screen when connected to WiFi, or you can access the web interface to see it.

### Step 2: Add Configuration to Home Assistant

1. Open your Home Assistant configuration file (`configuration.yaml`)
2. Copy the contents from `homeassistant-config.yaml` in this project
3. **Replace `192.168.1.XXX`** with your actual Az3166 board IP address (appears in multiple places)
4. Save the file

### Step 3: Restart Home Assistant

Restart Home Assistant to load the new configuration:
- Go to Settings → System → Restart
- Or use Developer Tools → YAML → Check Configuration → Restart

### Step 4: Add Dashboard Card

1. Go to your Home Assistant dashboard
2. Click the three dots (⋮) in the top right
3. Click "Edit Dashboard"
4. Click "+ Add Card"
5. Choose "Manual" and paste one of the card configurations from `homeassistant-dashboard.yaml`
6. Click "Save"

## Available Controls

### REST Commands (Backend Services)
These commands send HTTP requests to your Az3166 board:

- `rest_command.az3166_rgb_led_on` - Turn RGB LED on
- `rest_command.az3166_rgb_led_off` - Turn RGB LED off
- `rest_command.az3166_display_on` - Turn OLED display on
- `rest_command.az3166_display_off` - Turn OLED display off
- `rest_command.az3166_wifi_led_on` - Turn WiFi status LED on
- `rest_command.az3166_wifi_led_off` - Turn WiFi status LED off
- `rest_command.az3166_azure_led_on` - Turn Azure status LED on
- `rest_command.az3166_azure_led_off` - Turn Azure status LED off

### Button Entities (UI Elements)
These appear as buttons in your dashboard:

- `button.az3166_rgb_led_on`
- `button.az3166_rgb_led_off`
- `button.az3166_display_on`
- `button.az3166_display_off`

### Sensor Entities
These read telemetry data from your board:

- `sensor.az3166_temperature` - Temperature in °C
- `sensor.az3166_humidity` - Humidity in %
- `sensor.az3166_pressure` - Atmospheric pressure in mbar

## Using Buttons in Automations

You can use these buttons in Home Assistant automations:

```yaml
automation:
  - alias: "Turn Az3166 LED on at sunset"
    trigger:
      - platform: sun
        event: sunset
    action:
      - service: rest_command.az3166_rgb_led_on
      
  - alias: "Turn Az3166 LED off at sunrise"
    trigger:
      - platform: sun
        event: sunrise
    action:
      - service: rest_command.az3166_rgb_led_off
      
  - alias: "Alert when temperature too high"
    trigger:
      - platform: numeric_state
        entity_id: sensor.az3166_temperature
        above: 30
    action:
      - service: notify.mobile_app
        data:
          message: "Az3166 temperature is {{ states('sensor.az3166_temperature') }}°C!"
```

## Testing

### Test from Developer Tools
1. Go to Developer Tools → Services
2. Select service: `rest_command.az3166_rgb_led_on`
3. Click "Call Service"
4. Your board's RGB LED should turn on!

### Test from Terminal/Command Line
```bash
# Turn LED on
curl "http://192.168.1.XXX/led?state=on"

# Turn LED off
curl "http://192.168.1.XXX/led?state=off"

# Get telemetry data
curl "http://192.168.1.XXX/telemetry"
```

## Troubleshooting

### Buttons don't appear
- Make sure you restarted Home Assistant after adding the configuration
- Check Developer Tools → States to see if the entities exist
- Look at Home Assistant logs for errors: Settings → System → Logs

### Buttons don't work
- Verify the IP address is correct
- Test the URL directly in a browser: `http://192.168.1.XXX/led?state=on`
- Check that your Az3166 board is powered on and connected to WiFi
- Ensure Home Assistant can reach the board (same network, no firewall blocking)

### Sensors show "unavailable"
- The REST sensor platform polls periodically
- Wait a few minutes for the first update
- Check that the `/telemetry` endpoint returns JSON data
- Adjust `scan_interval` in the sensor configuration if needed

## Advanced: Using Node-RED

If you have Node-RED installed in Home Assistant, you can create flows:

1. Add an "inject" node (triggers on button click)
2. Add an "http request" node
3. Set URL to: `http://192.168.1.XXX/led?state=on`
4. Connect the nodes
5. Deploy and click the inject button

## IP Address Change?

If your Az3166 board gets a new IP address:
1. Update all URLs in `configuration.yaml`
2. Restart Home Assistant
3. Consider setting a static IP for the board in your router's DHCP settings
