from enum import Enum

class BaseEnum(Enum):
    @classmethod
    def get_display(cls, raw_id):
        try:
            for member in cls:
                if member.value[0] == raw_id:
                    return member.value[1]
            return "Unknown"
        except:
            return "Unknown"

class Course(BaseEnum):
    AkinaLake = (0, "Lake Akina")
    Myogi = (1, "Myogi")
    Akagi = (2, "Akagi")
    Akina = (3, "Akina")
    Irohazaka = (4, "Irohazaka")
    Tsukuba = (5, "Tsukuba")
    Happo = (6, "Happogahara")
    Nagao = (7, "Nagao")
    Tsubaki = (8, "Tsubaki Line")
    Usui = (9, "Usui")
    Sadamine = (10, "Sadamine")
    Tsuchisaka = (11, "Tsuchisaka")
    AkinaSnow = (12, "Akina (Snow)")
    Hakone = (13, "Hakone")
    Momiji = (14, "Momiji Line")
    Nanamagari = (15, "Nanamagari")
    Gunsai = (16, "Gunsai")
    Odawara = (17, "Odawara")
    TsukubaSnow = (18, "Tsukuba (Snow)")
    Coute101 = (19, "Coute101 (???)")
    TsuchisakaSnow = (20, "Tsuchisaka (Snow)")

    def __init__(self, id, display_name):
        self.id = id
        self.display_name = display_name

class Direction(BaseEnum):
    Forward = (0, "Downhill")
    Backward = (1, "Uphill")
    def __init__(self, id, display_name):
        self.id = id
        self.display_name = display_name

class Time(BaseEnum):
    Day = (0, "Day")
    Night = (1, "Night")
    def __init__(self, id, display_name):
        self.id = id
        self.display_name = display_name

class GameMode(BaseEnum):
    StoryMode = (0, "Story Mode")
    TimeAttack = (1, "Time Attack")
    TheoryOfStreets = (2, "Theory of Streets")
    Default = (-1, "Unknown")
    def __init__(self, id, display_name):
        self.id = id
        self.display_name = display_name

    @classmethod
    def _missing_(cls, value):
        print("[!] Missing GameMode value: ", value)
        return cls.Default

class GlobalState(BaseEnum):
    Boot = (0, "Booting screen")
    Title = (1, "Title screen")
    Menu = (3, "Main menu")
    Race = (4, "Race")
    Default = (-1, "Unknown")
    def __init__(self, id, display_name):
        self.id = id
        self.display_name = display_name

    @classmethod
    def _missing_(cls, value):
        print("[!] Missing GameMode value: ", value)
        return cls.Default
