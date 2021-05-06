import boto3
import json
import datetime
import time
# from datetime  import date
from datetime import datetime
import math

s3_client = boto3.client('s3')
dynamodb_client = boto3.client('dynamodb')

def lambda_handler(event, context):
    
    current_timestamp = round(time.time())
    #current_ms_timestamp = 1620057623000 # 2021/05/03 18:23:00 
    current_ms_timestamp = round(time.time() * 1000)
    print(current_ms_timestamp)
    
    last_hour_timestamp = round(current_timestamp - 3600)
    last_hour_ms_timestamp = round(current_ms_timestamp - 3.6e6) # 1 hour = 3,600,000 ms = 3.6 * 10^6
    
    # response is a dictionary
    response = dynamodb_client.scan(
        TableName='HenhouseDbTable',
        #IndexName='string',
        #Limit=123,
        Select='ALL_ATTRIBUTES',
        # ReturnConsumedCapacity='INDEXES'|'TOTAL'|'NONE',
        # TotalSegments=123,
        # Segment=123,
        #ProjectionExpression='id, val',
        FilterExpression='id >= :min AND id <= :max',
        # ExpressionAttributeNames={ 'string': 'string' },
        ExpressionAttributeValues={
            ':min': {
                'N': str(last_hour_ms_timestamp),
            },
            ':max': {
                'N': str(current_ms_timestamp),
            }
        },
        # ConsistentRead=True|False
    )
    
    
    temperature_list = []
    light_list = []
    for x in  response["Items"]:
        if x["topic"]["S"] == "sensor/temp" :
            temperature_list.append(int(x["val"]["M"]["val"]["S"]))
        elif x["topic"]["S"] == "sensor/light" :
            light_list.append(int(x["val"]["M"]["val"]["S"]))
    
    #tmp = temperature_list[0]["id"]["N"]
    
    temperature_list_lenght = len(temperature_list)
    light_list_lenght = len(light_list)

    #DynamoDB returns dictionary response ordered by arrival time, so last element of the list is the last inserted in the DB  
    last_temp = temperature_list[temperature_list_lenght - 1]
    last_light = light_list[light_list_lenght - 1]
    
    min_temp = min(temperature_list)
    min_light = min(light_list)
    
    max_temp = max(temperature_list)
    max_light = max(light_list)
    
    avg_temp = math.fsum(temperature_list)/temperature_list_lenght
    avg_light = math.fsum(light_list)/light_list_lenght
    
    """ Debug only
    print("Computed values are: ")
    print("Temperature")
    print("# of retrieved temperature: " + str(temperature_list_lenght))
    print("Last temperature is : " + str(last_temp))
    print("Min temperature is: " + str(min_temp))
    print("Max temperature is: " + str(max_temp))
    print("Avg temperature is: " + str(avg_temp))
    
    print("Light")
    print("# of retrieved Light: " + str(light_list_lenght))
    print("Last Light is : " + str(last_light))
    print("Min Light is: " + str(min_light))
    print("Max Light is: " + str(max_light))
    print("Avg Light is: " + str(avg_light))
    """
    

    a = """
    <!DOCTYPE html>
    <html>
        <head>
            <title>Henhouse - salvo</title>
        </head>
        
        <body>
            <h1>Henhouse - Salvo</h1>
            <h2> Sensors </h2>
            <h3> Last received values </h3>
            <p> Last temperature in CELSIUS is: {0} </p>
            <p> Last light in LUX is: {1} </p>
            <br>
            <h3> Aggregated values </h3>
            <p> Average temperature in CELSIUS is: {2} </p>
            <p> Average light in LUX is: {3} </p>
            <br>
            <p> Minimum temperature in CELSIUS is: {4} </p>
            <p> Minimum light in LUX is: {5} </p>
            <br>
            <p> Max temperature in CELSIUS is: {6} </p>
            <p> Max light in LUX is: {7} </p>
            
            <h3> All received values during last hour ({8} - {9}) </h3>
            <p> Temperature: {10} </p>
            <p> Light: {11} </p>
            <br> <br>
    
            <h2> Actuator</h2>
            <h3>Buzzer - Alarm</h3>
            <button type="button" onclick="remoteCommand(1)">Turn ON</button>
            <button type="button" onclick="remoteCommand(2)">Turn OFF</button>
            
            <h3>Fan</h3>
            <button type="button" onclick="remoteCommand(3)">Turn ON</button>
            <button type="button" onclick="remoteCommand(4)">Turn OFF</button>
            
            <h3>Lamp</h3>
            <button type="button" onclick="remoteCommand(5)">Turn ON</button>
            <button type="button" onclick="remoteCommand(6)">Turn OFF</button>
    """     
    a = a.format(
        last_temp, 
        last_light, 
        avg_temp, 
        avg_light, 
        min_temp, 
        min_light, 
        max_temp, 
        max_light, 
        datetime.fromtimestamp(last_hour_timestamp),
        datetime.fromtimestamp(current_timestamp),
        temperature_list, 
        light_list
    )
    
    js_script = """
            <script src="https://sdk.amazonaws.com/js/aws-sdk-2.899.0.min.js"></script>
            <script type="text/javascript">
                // Initialize the Amazon Cognito credentials provider
                AWS.config.region = 'us-east-1'; // Region
                AWS.config.credentials = new AWS.CognitoIdentityCredentials({
                    IdentityPoolId: 'us-east-1:229ea7b7-435c-45a4-973a-8ef8cbfb3913',
                });            
                
                function remoteCommand(thing){
                    var iotdata = new AWS.IotData(
                        {
                            endpoint: 'a2ltfi1iwu8oyf-ats.iot.us-east-1.amazonaws.com',
                            apiVersion: '2015-05-28'
                        }
                    );
                    
                    if (thing == 1){
                        var params = {
                            topic: 'actuator/buzzer', /* required */
                            payload: '{ "value": "RemoteON" }',
                            qos: '0'
                        };
                    }
                    else if (thing == 2){
                        var params = {
                            topic: 'actuator/buzzer', /* required */
                            payload: '{ "value": "RemoteOFF" }',
                            qos: '0'
                        };
                    }
                    else if (thing == 3){
                        var params = {
                            topic: 'actuator/fan', /* required */
                            payload: '{ "value": "RemoteON" }',
                            qos: '0'
                        };
                    }
                    else if (thing == 4){
                        var params = {
                            topic: 'actuator/fan', /* required */
                            payload: '{ "value": "RemoteOFF" }',
                            qos: '0'
                        };
                    }                    
                    if (thing == 5){
                        var params = {
                            topic: 'actuator/lamp', /* required */
                            payload: '{ "value": "RemoteON" }',
                            qos: '0'
                        };
                    }
                    else if (thing == 6){
                        var params = {
                            topic: 'actuator/lamp', /* required */
                            payload: '{ "value": "RemoteOFF" }',
                            qos: '0'
                        };
                    }
                    
                    iotdata.publish(
                        params, 
                        function(err, data) {
                            if (err){
                                console.log(err, err.stack); // an error occurred
                                alert("Error! Command not executed");
                            } 
                            else{
                                console.log(data);           // successful response
                                alert("OK! Remote command sent");
                            }     
                        }
                    );
                }
            </script>
        </body>
    </html> """
    
    
    html_page = a + js_script 
    #print(html_page)
    
    
    
      
    response = s3_client.put_object(
        ACL='public-read-write',
        Body=html_page,
        Bucket='henhouse-salvo',
        # ContentDisposition='string',
        # ContentEncoding='string',
        # ContentLanguage='string',
        # ContentLength=123,
        # ContentMD5='string',
        ContentType='text/html',
        # Expires=datetime(2015, 1, 1),
        # GrantFullControl='string',
        # GrantRead='string',
        # GrantReadACP='string',
        # GrantWriteACP='string',
        Key='index.html',
        # ServerSideEncryption='AES256'|'aws:kms',
        StorageClass='STANDARD',
        # WebsiteRedirectLocation='string',
        # SSECustomerAlgorithm='string',
        # SSECustomerKey='string',
        # SSEKMSKeyId='string',
        # SSEKMSEncryptionContext='string',
        # BucketKeyEnabled=True|False,
        # RequestPayer='requester',
        # Tagging='string',
        # ObjectLockMode='GOVERNANCE'|'COMPLIANCE',
        # ObjectLockRetainUntilDate=datetime(2015, 1, 1),
        # ObjectLockLegalHoldStatus='ON'|'OFF',
        # ExpectedBucketOwner='string'
    )
    print(response)
    
    
    return {
        'statusCode': 200,
        'body': json.dumps('Hello from Lambda!')
    }
