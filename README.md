# AirGradient Prometheus + JSON metrics
AirGradient sketch for prometheus &amp; json http poller parsing.

## How it works

This is a custom Arduino sketch you can upload to the D1 Mini board of the AirGradient open source version of their DIY PRO kit. 

- Follow the instruction to [setup the software](https://www.airgradient.com/open-airgradient/instructions/basic-setup-skills-and-equipment-needed-to-build-our-airgradient-diy-sensor/)
- Open the AirGradient_Prometheus_Json.ino in the Arduino editor. Click Upload
- Connect your board to your local WIFI and assign a static IP to your new device in your routeur software.
- Validate the endpoints are working by visiting: `http://your-local-ip:9926/metrics/json` && `http://your-local-ip:9926/metrics/prometheus`


## Prometheus

Install prometheus on a local server on your network and use the following configuration to start scraping the data.:

```yaml
scrape_configs:
  - job_name: 'airgradient-UNIQUE-DEVICE-ID'
    metrics_path: /metrics/prometheus
    scrape_interval: 30s
    static_configs:
      - targets: ['LOCAL-IP_HERE:9926']
```

## Grafana

To visualize the prometheus data in a Grafana instance of your local network you can use the reference [grafana dashboard](./grafana/dashboard.json) after having configured the Prometheus Data source in Grafana.

## Credit

Code adapted from [@GeerlingGuy](https://github.com/geerlingguy/airgradient-prometheus) to work with latest V3.7 of the [DIY PRO kit](https://www.airgradient.com/open-airgradient/instructions/diy-pro-presoldered-v37/).

Code also adapted from [Arduino Reference Sketch](https://github.com/airgradienthq/arduino)
