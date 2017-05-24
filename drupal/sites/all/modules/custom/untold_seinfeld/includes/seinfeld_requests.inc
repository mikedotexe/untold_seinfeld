<?php

/**
 * Helper function - make cURL request
 */
function _seinfeld_curl_request(&$ch, $uri, $method) {
  global $base_url;
  $api_env = variable_get('tms_chosen_api');
  $api_path = variable_get('tms_sp_' . $api_env . '_api');
  $environment = variable_get('environment');

  $url = "$api_path/$uri";

  curl_setopt($ch, CURLOPT_URL, $url);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
  // since we use Basic Auth on the Forge server squanch, we'll add that into the curl request if needed
  if ($api_path == $base_url . '/api/mock' && variable_get('tms_squanch_use_auth')) {
    if ($environment == 'dev-untold' || $environment == 'dev-release') {
      curl_setopt($ch, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
      $squanch_uname = variable_get('tms_squanch_username');
      $squanch_pw = variable_get('tms_squanch_password');
      curl_setopt($ch, CURLOPT_USERPWD, "$squanch_uname:$squanch_pw");
    }
  }
  if (variable_get('tms_sp_' . $api_env . '_use_auth')) {
    curl_setopt($ch, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    $sp_username = variable_get('tms_sp_' . $api_env . '_username');
    $sp_password = variable_get('tms_sp_' . $api_env . '_password');
    curl_setopt($ch, CURLOPT_USERPWD, "$sp_username:$sp_password");
  }
  curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: application/xml'));
  if (variable_get('tms_disable_verify_ssl', FALSE)) {
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, FALSE);
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, FALSE);
  }

  if ($method == 'GET') {
    curl_setopt($ch, CURLOPT_HTTPGET, TRUE);
  } else {
    watchdog('tms', 'Non-GET method received in _curl_request going to %url', array('%url' => $url), WATCHDOG_ERROR);
  }

  // Execute & get variables
  try {
    $api_response = curl_exec($ch);
  } catch(Exception $e) {
    watchdog('tms', 'Curl failed with error %code: %msg', array('%code' => $e->getCode(), '%msg' => $e->getMessage()));
  }
  $curlError = curl_error($ch);
  $info = curl_getinfo($ch);
  $http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
  $success = _seinfeld_check_curl_success($api_response, $curlError, $http_code, $info, $uri);
  if ($success) {
    return $api_response;
  } else {
    return FALSE;
  }
}

/**
 * Helper function - test if there was a cURL problem (request didn't go through)
 */
function _seinfeld_check_curl_success($api_response, $curlError, $http_code, $info, $uri) {
  if ($curlError) {
    watchdog('tms', 'There was a problem making the API Call. cURL problem. Code %code with messsage %message with info: %info for uri: %uri', array('%code' => $http_code, '%message' => $curlError, '%info' => $info, '%uri' => $uri), WATCHDOG_ERROR);
    if ($http_code == 200) {
      // we've logged the error, if it's a 200 carry on
      return TRUE;
    } else {
      return FALSE;
    }
  }
  if ($http_code == 200) {
    // successful response or Bad Request which will return error details
    return TRUE;
  } else if ($http_code == 400) {
    // 400 return valuable information, so we'll return true, but we want to include a report in watchdog
    watchdog('tms', 'Received %code status code with info of %info for uri %uri with response of %resp', array('%code' => $http_code, '%info' => $info, '%uri' => $uri, '%resp' => $api_response), WATCHDOG_ERROR);
    return TRUE;
  } else {
    // request error
    watchdog('tms', 'Did not receive 200 or 400 http status code. Instead received %code and info of %info for uri %uri with response of %resp', array('%code' => $http_code, '%info' => $info, '%uri' => $uri, '%resp' => $api_response), WATCHDOG_ERROR);
    return FALSE;
  }
}
