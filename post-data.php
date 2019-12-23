<?php
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

$servername = "localhost";

// REPLACE with your Database name
$dbname = "id11942892_esp_data";
// REPLACE with Database user
$username = "id11942892_esp_board";
// REPLACE with Database user password
$password = "123456";

// Keep this API Key value to be compatible with the ESP32 code provided in the project page. If you change this value, the ESP32 sketch needs to match

$api_key_value = "tPmAT5Ab3j7F9";

$api_key  = $value1 = $value2 = $value3 = "";


        // Create connection
        $conn = new mysqli($servername, $username, $password, $dbname);

$value1 =$_GET["value1"];
$value2 =$_GET["value2"];
$value3 =$_GET["value3"];  
        $sql = "INSERT INTO Sensor (value1, value2, value3)
        VALUES ('" . $value1 . "', '" . $value2 . "', '" . $value3 . "')";
        
        if ($conn->query($sql) === TRUE) {
            echo "New record created successfully";
        } 
        else {
            echo "Error: " . $sql . "<br>" . $conn->error;
        }
    
        $conn->close();
    ?>