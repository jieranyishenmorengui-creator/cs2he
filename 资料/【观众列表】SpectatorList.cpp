    public static class SpectatorList
    {
        private static readonly List<string> _currentSpectators = new();
        public static bool ShowSpectators { get; set; } = true;
 
        public enum SpectatorMode
        {
            None = 0,
            DeathCam = 1,
            FreezeCam = 2,
            Fixed = 3,
            FirstPerson = 4,
            ThirdPerson = 5,
            FreeRoam = 6
        }
 
        public static void Update(GameData gameData)
        {
            if (!ShowSpectators || gameData.Player.AddressBase == IntPtr.Zero) return;
            
            _currentSpectators.Clear();
 
            foreach (var entity in gameData.Entities)
            {
                if (entity.AddressBase == gameData.Player.AddressBase)
                    continue;
 
                if ( 
                    !entity.IsAlive() && 
                    IsSpectatingLocalPlayer(gameData, entity))
                {
                    var spectatorName = entity.Name;
                    if (!string.IsNullOrEmpty(spectatorName))
                    {
                        _currentSpectators.Add(spectatorName);
                        Console.WriteLine($"Added spectator: {spectatorName}");
                    }
                }
            }
        }
 
        private static bool IsSpectatingLocalPlayer(GameData gameData, Entity entity)
        {
            try
            {
                var observerServices = gameData.Process.Process.Read<IntPtr>(entity.AddressBase + Offsets.m_pObserverServices);
                if (observerServices == IntPtr.Zero)
                {
                    Console.WriteLine($"Observer services is null for {entity.Name}");
                    return false;
                }
 
                var observerTarget = gameData.Process.Process.Read<IntPtr>(observerServices + Offsets.m_hObserverTarget);
                var targetBase = ReadAddressBase(gameData, observerTarget);
 
                return targetBase == gameData.Player.AddressBase;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error checking spectator status: {ex.Message}");
                return false;
            }
        }
 
        private static IntPtr ReadAddressBase(GameData gameData, IntPtr playerPawn)
        {
            try
            {
                var entityList = gameData.Process.Process.Read<IntPtr>(gameData.Process.ModuleClient.ProcessModule.BaseAddress + Offsets.dwEntityList);
                var listEntry = gameData.Process.Process.Read<IntPtr>(entityList + (nint)(0x8 * ((playerPawn.ToInt64() & 0x7FFF) >> 9) + 16));
                
                if (listEntry == IntPtr.Zero)
                    return IntPtr.Zero;
 
                return gameData.Process.Process.Read<IntPtr>(listEntry + (nint)(120 * (playerPawn.ToInt64() & 0x1FF)));
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error reading address base: {ex.Message}");
                return IntPtr.Zero;
            }
        }
 
        public static List<string> GetCurrentSpectators()
        {
            return new List<string>(_currentSpectators);
        }
    }