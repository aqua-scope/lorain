function decodeUplink(input) {
	bytes = input.bytes;
	var data = {};
  	for (i=0 ; i<bytes.length; i++) {
    	switch (bytes[i]) {
      		case 0x03:
        		data.hw_version = bytes[++i];
        		data.capabilities = (bytes[++i] << 8)+  bytes[++i];
        		break;
      		case 0x04:
        		p = bytes[++i];
        		v = (bytes[++i] << 8)+  bytes[++i];
        		switch (p) {
				case 0x02:
					data.conf_heartbeat=v;
        	        break;
   		     	case 0x03:
        	    	data.conf_heavyrain=v;
            	    break;
        		case 0x04:
        			data.conf_interval=v;
            		break;
            	default:
                	data.error = "config parameter? "+bytes[i];
        		}
        	break;
        	case 0x06:
        		sensor = bytes[++i];
            	sensorvalue = (bytes[++i] << 8)+  bytes[++i];
            	switch(sensor) {
            	case 0x01:
                	data.temperature = sensorvalue;
                    break;
            	case 0x03:
                	data.uptime = sensorvalue;
                    break;
            	case 0x81:
                	data.rainlevel = sensorvalue;
                    break;
                default:
                    data.error = "sensor type? "+bytes[i];
            	}
        	break;
        case 0x0a:
            data.fw_version = (bytes[++i] << 24) +(bytes[++i] << 16) + (bytes[++i] << 8) + bytes[++i];
            break;
        case 0x0b:
            data.alarm_status = bytes[++i];
            data.alarm_type = bytes[++i];
            data.alarm_value = (bytes[++i] << 8)+  bytes[++i];
            break;
        case 0x12:
            data.bat_voltage = bytes[++i];
            data.bat_consumption = (bytes[++i] << 8) + bytes[++i];        
            break;
        default:
            data.error = "command? "+bytes[i];
        }
    }  
    return {
    data: data,
    warnings: [],
    errors: []
  };  
}