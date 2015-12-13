<?php

class MsaController extends EntityAPIController {

  protected function buildQuery($ids, $conditions = array(), $revision_id = FALSE) {
    $join_fields = array('zip','city','county','state');

    foreach ($conditions as $field => $value) {
      if (in_array($field, $join_fields)) {
        ${$field} = $value;
        unset($conditions[$field]);
      }
    }
    $query = parent::buildQuery($ids, $conditions, $revision_id);

    foreach ($join_fields as $field) {
      if (isset(${$field})) {
        $query->join($this->entityInfo["$field table"], $field, "base.cbsa = $field.cbsa");
        $cond_value = (($field == 'zip') ? ${$field} : '%' . ${$field} . '%');
        $query->condition("$field.$field", $cond_value, ($field == 'zip' ? '=' : 'LIKE'));
      }
    }

    return $query;
  }

  /**
   * @inheritdoc
   */
  protected function attachLoad(&$queried_entities, $revision_id = FALSE) {
    // We have to attach principal cities and the zip list before running load
    // hooks or field attachments.
    $cities = db_select($this->entityInfo['city table'], 'city')
      ->fields('city')
      ->condition("city.cbsa", array_keys($queried_entities), 'IN')
      ->execute();
    foreach ($cities->fetchAll() as $city) {
      $queried_entities[$city->cbsa]->addCity($city);
    }

    $counties = db_select($this->entityInfo['county table'], 'county')
      ->fields('county')
      ->condition("county.cbsa", array_keys($queried_entities), 'IN')
      ->execute();
    foreach ($counties->fetchAll() as $county) {
      $queried_entities[$county->cbsa]->addCounty($county);
    }

    $zips = db_select($this->entityInfo['zip table'], 'zip')
      ->fields('zip')
      ->condition("zip.cbsa", array_keys($queried_entities), 'IN')
      ->execute();
    foreach ($zips->fetchAll() as $zip) {
      $queried_entities[$zip->cbsa]->addZip($zip);
    }

    // Let the field attach process and load hooks run now that our objects are
    // in their completed state.
    parent::attachLoad($queried_entities, $revision_id);
  }

}
