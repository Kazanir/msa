<?php

class Msa extends Entity {

  protected $cities = array();
  protected $counties = array();
  protected $zips = array();

  /**
   * Adds a city to the cities list, keyed by the city FIPS code.
   *
   * @param $city An object containing a city record from the database.
   */
  public function addCity($city) {
    $fips = $city->fips_state . $city->fips_place;
    unset($city->cbsa);
    $this->cities[$fips] = $city;

    return $this;
  }

  /**
   * Adds a county to the counties list, keyed by the county FIPS code.
   *
   * @param $city An object containing a city record from the database.
   */
  public function addZip($zip) {
    unset($zip->cbsa);
    $this->zips[$zip->zip] = $zip;

    return $this;
  }

  /**
   * Adds a zip to the zipcodes list, keyed by the zip code.
   *
   * @param $city An object containing a city record from the database.
   */
  public function addCounty($county) {
    unset($county->cbsa);
    $fips = $county->fips_state . $county->fips_county;
    $this->counties[$fips] = $county;

    return $this;
  }

  /**
   * Allows cities, counties, and zips to be accessed normally without being
   * publicly modifiable.
   */
  public function __get($name) {
    if (isset($this->{$name})) {
      return $this->{$name};
    }
  }

}

