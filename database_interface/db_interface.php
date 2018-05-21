<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>SSL_EXPERIMENT_VER2.0</title>
</head>

<body>


<?php

    @$db = new mysqli('localhost', 'USER_NAME', 'PASSWORD', 'DATABASE');

    if (mysqli_connect_errno()) {
        echo "Error: Could not connect to database.  Please try again later.";
        exit;
    }
	$uncompleted_mark = false;
	if(isset($_POST['tasklabel']))
	{
		$TASK_LABEL = $_POST['tasklabel'];

	}else{$uncompleted_mark = true;}
	if(isset($_POST['servername']))
	{
		$SERVER_NAME = $_POST['servername'];

	}else{$uncompleted_mark = true;}
	if(isset($_POST['serverip']))
	{
		$SERVER_IP = $_POST['serverip'];
	}else{$uncompleted_mark = true;}
	if(isset($_POST['webtopdomain']))
	{
		$TARGET_WEBSITE_TOP_DOMAIN = $_POST['webtopdomain'];
	}else{$uncompleted_mark = true;}
	if(isset($_POST['websourcedomain']))
	{
		$TARGET_WEBSITE_SOURCE_DOMAIN = $_POST['websourcedomain'];
	}else{$uncompleted_mark = true;}
	if(isset($_POST['webdomain']))
	{
		$TARGET_WEBSITE_DOMAIN = $_POST['webdomain'];
	}else{$uncompleted_mark = true;}
	if(isset($_POST['webip']))
	{
		$TARGET_WEBSITE_IP = $_POST['webip'];
	}else{$uncompleted_mark = true;}
	if(isset($_POST['webdepth']))
	{
		$TARGET_WEBSITE_DEPTH = $_POST['webdepth'];
	}else{$uncompleted_mark = true;}
	if(isset($_POST['webrtt']))
	{
		$TARGET_WEBSITE_RTT = $_POST['webrtt'];
	}else{$uncompleted_mark = true;}
	if(isset($_POST['webtype']))
	{
		$TARGET_WEBSITE_TYPE = $_POST['webtype'];
	}else{$uncompleted_mark = true;}
	if(isset($_POST['cert']))
	{
		$TARGET_WEBSITE_CERT = $_POST['cert'];
	}else{$uncompleted_mark = true;}
	if(isset($_POST['exten']))
	{
		$TARGET_WEBSITE_EXTEN = $_POST['exten'];
	}else{$uncompleted_mark = true;}


	if($uncompleted_mark == true)
	{
		echo 'not enough variables';
		exit();
	}
	
	$addCommand = 'Insert into TABLE_CERT_RAW_SIMON(TASK_LABEL, SERVER_NAME, SERVER_IP, TARGET_WEBSITE_TOP_DOMAIN, TARGET_WEBSITE_SOURCE_DOMAIN, TARGET_WEBSITE_DOMAIN, TARGET_WEBSITE_IP, TARGET_WEBSITE_DEPTH, TARGET_WEBSITE_RTT, TARGET_WEBSITE_TYPE, TARGET_WEBSITE_CERT, TARGET_WEBSITE_CERT_MD5, EXTENSION)';
	$addCommand = $addCommand."VALUES('".$TASK_LABEL."','".$SERVER_NAME."','".$SERVER_IP."','".$TARGET_WEBSITE_TOP_DOMAIN."','".$TARGET_WEBSITE_SOURCE_DOMAIN."','".$TARGET_WEBSITE_DOMAIN."','".$TARGET_WEBSITE_IP."',".$TARGET_WEBSITE_DEPTH.",".$TARGET_WEBSITE_RTT.",".$TARGET_WEBSITE_TYPE.",'".$TARGET_WEBSITE_CERT."',MD5('".$TARGET_WEBSITE_CERT."'),'".$TARGET_WEBSITE_EXTEN."')";
    echo $addCommand;

    $db->query($addCommand);
	echo 'command has been executed.';

?>


</body>
</html>