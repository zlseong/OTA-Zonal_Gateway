/******************************************************************************
 * @file     ota_manager.c
 * @brief    OTA Package Manager Implementation
 * @version  1.0.0
 * @date     2025-11-05
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "ota_manager.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Initialize OTA Manager
 *****************************************************************************/
OTA_Result OTA_Manager_Init(OTA_Manager *manager, SPI_Flash_S25FL512S *flash)
{
    if (manager == NULL || flash == NULL)
    {
        return OTA_ERROR_INVALID_PARAMETER;
    }
    
    if (!flash->is_initialized)
    {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    /* Clear manager */
    memset(manager, 0, sizeof(OTA_Manager));
    manager->flash = flash;
    
    /* Read metadata from flash */
    SPI_Flash_Result flash_result = SPI_Flash_Read(flash, 
                                                     OTA_METADATA_ADDRESS,
                                                     (uint8 *)&manager->metadata,
                                                     sizeof(OTA_Metadata));
    
    if (flash_result != FLASH_OK)
    {
        return OTA_ERROR_FLASH_ERROR;
    }
    
    /* Check if metadata is valid */
    if (manager->metadata.magic == OTA_MAGIC && 
        manager->metadata.version == OTA_VERSION)
    {
        /* Valid metadata found */
        manager->is_initialized = TRUE;
        return OTA_OK;
    }
    
    /* Initialize new metadata */
    memset(&manager->metadata, 0, sizeof(OTA_Metadata));
    manager->metadata.magic = OTA_MAGIC;
    manager->metadata.version = OTA_VERSION;
    manager->metadata.package_count = 0;
    manager->metadata.free_space_start = OTA_DATA_START_ADDRESS;
    manager->metadata.free_space_size = S25FL512S_CAPACITY_BYTES - OTA_DATA_START_ADDRESS;
    manager->metadata.total_packages_received = 0;
    manager->metadata.total_bytes_received = 0;
    
    /* Save to flash */
    OTA_Result result = OTA_Manager_SaveMetadata(manager);
    if (result == OTA_OK)
    {
        manager->is_initialized = TRUE;
    }
    
    return result;
}

/******************************************************************************
 * Allocate Space for New Package
 *****************************************************************************/
OTA_Result OTA_Manager_AllocatePackage(OTA_Manager *manager, 
                                        uint32 size, 
                                        uint32 *address)
{
    if (manager == NULL || !manager->is_initialized || address == NULL)
    {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    if (size == 0 || size > manager->metadata.free_space_size)
    {
        /* Try garbage collection */
        OTA_Manager_GarbageCollect(manager);
        
        /* Check again */
        if (size > manager->metadata.free_space_size)
        {
            return OTA_ERROR_NO_SPACE;
        }
    }
    
    /* Check if package table is full */
    if (manager->metadata.package_count >= OTA_MAX_PACKAGES)
    {
        /* Try garbage collection */
        OTA_Manager_GarbageCollect(manager);
        
        /* Check again */
        if (manager->metadata.package_count >= OTA_MAX_PACKAGES)
        {
            return OTA_ERROR_PACKAGE_TABLE_FULL;
        }
    }
    
    /* Allocate at free_space_start */
    *address = manager->metadata.free_space_start;
    
    /* Update free space pointers */
    manager->metadata.free_space_start += size;
    manager->metadata.free_space_size -= size;
    
    return OTA_OK;
}

/******************************************************************************
 * Add Package Entry
 *****************************************************************************/
OTA_Result OTA_Manager_AddPackage(OTA_Manager *manager,
                                   const char *target_ecu_id,
                                   uint8 target_type,
                                   uint8 bus_type,
                                   uint8 condition,
                                   uint32 flash_addr,
                                   uint32 package_size,
                                   uint32 firmware_crc)
{
    if (manager == NULL || !manager->is_initialized || target_ecu_id == NULL)
    {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    if (manager->metadata.package_count >= OTA_MAX_PACKAGES)
    {
        return OTA_ERROR_PACKAGE_TABLE_FULL;
    }
    
    /* Find empty slot */
    uint8 slot = 0xFF;
    for (uint8 i = 0; i < OTA_MAX_PACKAGES; i++)
    {
        if (manager->metadata.packages[i].status == OTA_STATUS_EMPTY)
        {
            slot = i;
            break;
        }
    }
    
    if (slot == 0xFF)
    {
        return OTA_ERROR_PACKAGE_TABLE_FULL;
    }
    
    /* Fill entry */
    OTA_PackageEntry *entry = &manager->metadata.packages[slot];
    memset(entry, 0, sizeof(OTA_PackageEntry));
    
    entry->flash_address = flash_addr;
    entry->package_size = package_size;
    
    strncpy(entry->target_ecu_id, target_ecu_id, OTA_MAX_ECU_ID_LEN - 1);
    entry->target_ecu_id[OTA_MAX_ECU_ID_LEN - 1] = '\0';  /* Ensure null-terminated */
    
    entry->target_type = target_type;
    entry->bus_type = bus_type;
    entry->condition = condition;
    entry->status = OTA_STATUS_COMPLETE;
    entry->firmware_crc32 = firmware_crc;
    
    /* Get timestamp (use 0 for now - can be improved with RTC) */
    entry->timestamp = 0;
    
    /* Update statistics */
    manager->metadata.package_count++;
    manager->metadata.total_packages_received++;
    manager->metadata.total_bytes_received += package_size;
    
    /* Save metadata */
    return OTA_Manager_SaveMetadata(manager);
}

/******************************************************************************
 * Find Package by ECU ID
 *****************************************************************************/
OTA_Result OTA_Manager_FindPackage(OTA_Manager *manager,
                                    const char *target_ecu_id,
                                    OTA_PackageEntry *entry_out)
{
    if (manager == NULL || !manager->is_initialized || target_ecu_id == NULL || entry_out == NULL)
    {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    for (uint8 i = 0; i < OTA_MAX_PACKAGES; i++)
    {
        OTA_PackageEntry *entry = &manager->metadata.packages[i];
        
        if (entry->status != OTA_STATUS_EMPTY &&
            strncmp(entry->target_ecu_id, target_ecu_id, OTA_MAX_ECU_ID_LEN) == 0)
        {
            /* Found */
            memcpy(entry_out, entry, sizeof(OTA_PackageEntry));
            return OTA_OK;
        }
    }
    
    return OTA_ERROR_PACKAGE_NOT_FOUND;
}

/******************************************************************************
 * Update Package Status
 *****************************************************************************/
OTA_Result OTA_Manager_UpdatePackageStatus(OTA_Manager *manager,
                                            const char *target_ecu_id,
                                            OTA_PackageStatus new_status)
{
    if (manager == NULL || !manager->is_initialized || target_ecu_id == NULL)
    {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    for (uint8 i = 0; i < OTA_MAX_PACKAGES; i++)
    {
        OTA_PackageEntry *entry = &manager->metadata.packages[i];
        
        if (entry->status != OTA_STATUS_EMPTY &&
            strncmp(entry->target_ecu_id, target_ecu_id, OTA_MAX_ECU_ID_LEN) == 0)
        {
            /* Found - update status */
            entry->status = new_status;
            return OTA_Manager_SaveMetadata(manager);
        }
    }
    
    return OTA_ERROR_PACKAGE_NOT_FOUND;
}

/******************************************************************************
 * Delete Package
 *****************************************************************************/
OTA_Result OTA_Manager_DeletePackage(OTA_Manager *manager,
                                      const char *target_ecu_id)
{
    if (manager == NULL || !manager->is_initialized || target_ecu_id == NULL)
    {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    for (uint8 i = 0; i < OTA_MAX_PACKAGES; i++)
    {
        OTA_PackageEntry *entry = &manager->metadata.packages[i];
        
        if (entry->status != OTA_STATUS_EMPTY &&
            strncmp(entry->target_ecu_id, target_ecu_id, OTA_MAX_ECU_ID_LEN) == 0)
        {
            /* Found - mark as empty */
            entry->status = OTA_STATUS_EMPTY;
            manager->metadata.package_count--;
            return OTA_Manager_SaveMetadata(manager);
        }
    }
    
    return OTA_ERROR_PACKAGE_NOT_FOUND;
}

/******************************************************************************
 * Garbage Collection
 *****************************************************************************/
OTA_Result OTA_Manager_GarbageCollect(OTA_Manager *manager)
{
    if (manager == NULL || !manager->is_initialized)
    {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    uint8 removed_count = 0;
    
    for (uint8 i = 0; i < OTA_MAX_PACKAGES; i++)
    {
        OTA_PackageEntry *entry = &manager->metadata.packages[i];
        
        /* Remove deployed packages */
        if (entry->status == OTA_STATUS_DEPLOYED)
        {
            entry->status = OTA_STATUS_EMPTY;
            manager->metadata.package_count--;
            removed_count++;
        }
    }
    
    if (removed_count > 0)
    {
        /* If all packages are gone, reset free space */
        if (manager->metadata.package_count == 0)
        {
            manager->metadata.free_space_start = OTA_DATA_START_ADDRESS;
            manager->metadata.free_space_size = S25FL512S_CAPACITY_BYTES - OTA_DATA_START_ADDRESS;
        }
        
        return OTA_Manager_SaveMetadata(manager);
    }
    
    return OTA_OK;
}

/******************************************************************************
 * Save Metadata to Flash
 *****************************************************************************/
OTA_Result OTA_Manager_SaveMetadata(OTA_Manager *manager)
{
    if (manager == NULL || !manager->is_initialized)
    {
        return OTA_ERROR_NOT_INITIALIZED;
    }
    
    /* Erase metadata sector (4KB) */
    SPI_Flash_Result result = SPI_Flash_EraseSector(manager->flash, OTA_METADATA_ADDRESS);
    if (result != FLASH_OK)
    {
        return OTA_ERROR_FLASH_ERROR;
    }
    
    /* Write metadata */
    result = SPI_Flash_Write(manager->flash,
                             OTA_METADATA_ADDRESS,
                             (uint8 *)&manager->metadata,
                             sizeof(OTA_Metadata));
    
    if (result != FLASH_OK)
    {
        return OTA_ERROR_FLASH_ERROR;
    }
    
    return OTA_OK;
}

/******************************************************************************
 * Get Statistics
 *****************************************************************************/
void OTA_Manager_GetStatistics(OTA_Manager *manager,
                                uint16 *package_count,
                                uint32 *free_space,
                                uint32 *total_received)
{
    if (manager == NULL || !manager->is_initialized)
    {
        if (package_count) *package_count = 0;
        if (free_space) *free_space = 0;
        if (total_received) *total_received = 0;
        return;
    }
    
    if (package_count) *package_count = manager->metadata.package_count;
    if (free_space) *free_space = manager->metadata.free_space_size;
    if (total_received) *total_received = manager->metadata.total_packages_received;
}

/******************************************************************************
 * Get Error String
 *****************************************************************************/
const char* OTA_Manager_GetErrorString(OTA_Result result)
{
    switch (result)
    {
        case OTA_OK:                            return "OK";
        case OTA_ERROR_NOT_INITIALIZED:         return "Not initialized";
        case OTA_ERROR_INVALID_PARAMETER:       return "Invalid parameter";
        case OTA_ERROR_NO_SPACE:                return "No space available";
        case OTA_ERROR_PACKAGE_TABLE_FULL:      return "Package table full";
        case OTA_ERROR_FLASH_ERROR:             return "Flash error";
        case OTA_ERROR_PACKAGE_NOT_FOUND:       return "Package not found";
        case OTA_ERROR_METADATA_CORRUPT:        return "Metadata corrupt";
        case OTA_ERROR_MAGIC_MISMATCH:          return "Magic mismatch";
        default:                                return "Unknown error";
    }
}


