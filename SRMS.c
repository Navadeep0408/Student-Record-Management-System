#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
  #include <direct.h> /* _mkdir */
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

#define STUDENT_FILE "students.txt"
#define CREDENTIAL_FILE "credentials.txt"
#define BACKUP_DIR "backups"
#define LINE_BUF_SIZE 512

struct Student {
    int roll;
    char name[200];
    float marks;
};

char currentRole[32];
char currentUser[64];

/* function prototypes */
int loginSystem(void);
void mainMenu(void);
void adminMenu(void);
void userMenu(void);
void staffMenu(void);
void guestMenu(void);

void addStudent(void);
void displayStudents(void);
void searchStudent(void);
void updateStudent(void);
void deleteStudent(void);

int read_student_line(FILE *fp, struct Student *s);
void append_student_line(FILE *fp, const struct Student *s);
int roll_exists(int roll);
int make_backup(const char *filename);
void clear_stdin(void);

/* ---------------- main ---------------- */
int main(void) {
    printf("Student Record Management\n");
    if (loginSystem()) {
        mainMenu();
    } else {
        printf("\nAccess Denied. Exiting..\n");
    }
    return 0;
}

/* ---------------- login ---------------- */
int loginSystem(void) {
    char username[64], password[64];
    char fileUser[64], filePass[64], fileRole[32];
    int ok = 0;

    printf("===== Login =====\n");
    printf("Username: ");
    if (scanf("%63s", username) != 1) return 0;
    printf("Password: ");
    if (scanf("%63s", password) != 1) return 0;
    clear_stdin();

    FILE *fp = fopen(CREDENTIAL_FILE, "r");
    if (!fp) {
        printf("Error: %s not found!\n", CREDENTIAL_FILE);
        return 0;
    }

    while (fscanf(fp, "%63s %63s %31s", fileUser, filePass, fileRole) == 3) {
        if (strcmp(username, fileUser) == 0 && strcmp(password, filePass) == 0) {
            strncpy(currentRole, fileRole, sizeof(currentRole)-1);
            currentRole[sizeof(currentRole)-1] = '\0';
            strncpy(currentUser, fileUser, sizeof(currentUser)-1);
            currentUser[sizeof(currentUser)-1] = '\0';
            ok = 1;
            break;
        }
    }
    fclose(fp);

    if (ok) {
        printf("Login successful. Role: %s, User: %s\n", currentRole, currentUser);
    }
    return ok;
}

/* ---------------- menus ---------------- */
void mainMenu(void) {
    if (strcmp(currentRole, "ADMIN") == 0) adminMenu();
    else if (strcmp(currentRole, "USER") == 0) userMenu();
    else if (strcmp(currentRole, "STAFF") == 0) staffMenu();
    else guestMenu();
}

void adminMenu(void) {
    int choice;
    do {
        printf("\n===== ADMIN MENU =====\n");
        printf("1. Add Student\n");
        printf("2. Display Students\n");
        printf("3. Search Student\n");
        printf("4. Update Student\n");
        printf("5. Delete Student\n");
        printf("6. Logout\n");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) { clear_stdin(); choice = -1; }
        clear_stdin();
        switch (choice) {
            case 1: addStudent(); break;
            case 2: displayStudents(); break;
            case 3: searchStudent(); break;
            case 4: updateStudent(); break;
            case 5: deleteStudent(); break;
            case 6: printf("Logging out...\n"); return;
            default: printf("Invalid choice!\n");
        }
    } while (1);
}

void userMenu(void) {
    int ch;
    do {
        printf("\n===== USER MENU =====\n1. Display Students\n2. Search Student\n3. Logout\nEnter choice: ");
        if (scanf("%d", &ch) != 1) { clear_stdin(); ch = -1; }
        clear_stdin();
        if (ch == 1) displayStudents();
        else if (ch == 2) searchStudent();
        else if (ch == 3) { printf("Logging out...\n"); return; }
        else printf("Invalid choice!\n");
    } while (1);
}

void staffMenu(void) {
    int ch;
    do {
        printf("\n===== STAFF MENU =====\n1. Display Students\n2. Search Student\n3. Update Student\n4. Logout\nEnter choice: ");
        if (scanf("%d", &ch) != 1) { clear_stdin(); ch = -1; }
        clear_stdin();
        if (ch == 1) displayStudents();
        else if (ch == 2) searchStudent();
        else if (ch == 3) updateStudent();
        else if (ch == 4) { printf("Logging out...\n"); return; }
        else printf("Invalid choice!\n");
    } while (1);
}

void guestMenu(void) {
    int ch;
    do {
        printf("\n===== GUEST MENU =====\n1. Display Students\n2. Logout\nEnter choice: ");
        if (scanf("%d", &ch) != 1) { clear_stdin(); ch = -1; }
        clear_stdin();
        if (ch == 1) displayStudents();
        else if (ch == 2) { printf("Logging out...\n"); return; }
        else printf("Invalid choice!\n");
    } while (1);
}

/* ---------------- student operations ---------------- */
void addStudent(void) {
    struct Student s;
    char namebuf[200];

    printf("\nAdd Student\nEnter roll (int): ");
    if (scanf("%d", &s.roll) != 1) { printf("Invalid roll\n"); clear_stdin(); return; }
    clear_stdin();
    if (s.roll <= 0) { printf("Roll must be positive.\n"); return; }

    if (roll_exists(s.roll)) {
        printf("Error: roll %d already exists. Use update to modify.\n", s.roll);
        return;
    }

    printf("Enter full name: ");
    if (!fgets(namebuf, sizeof(namebuf), stdin)) { printf("Name read error\n"); return; }
    size_t ln = strlen(namebuf);
    if (ln && namebuf[ln-1] == '\n') namebuf[ln-1] = '\0';
    if (strlen(namebuf) == 0) { printf("Name cannot be empty\n"); return; }
    strncpy(s.name, namebuf, sizeof(s.name)-1);
    s.name[sizeof(s.name)-1] = '\0';

    printf("Enter marks (float): ");
    if (scanf("%f", &s.marks) != 1) { printf("Invalid marks\n"); clear_stdin(); return; }
    clear_stdin();

    FILE *fp = fopen(STUDENT_FILE, "a");
    if (!fp) { printf("Could not open %s for append: %s\n", STUDENT_FILE, strerror(errno)); return; }
    append_student_line(fp, &s);
    fclose(fp);
    printf("Student added successfully.\n");
}

void displayStudents(void) {
    struct Student s;
    FILE *fp = fopen(STUDENT_FILE, "r");
    if (!fp) {
        printf("%s not found or no students yet.\n", STUDENT_FILE);
        return;
    }
    printf("\n--- Students List ---\n");
    printf("%-8s %-30s %6s\n", "Roll", "Name", "Marks");
    while (read_student_line(fp, &s)) {
        printf("%-8d %-30s %6.2f\n", s.roll, s.name, s.marks);
    }
    fclose(fp);
}

void searchStudent(void) {
    int target; struct Student s; int found = 0;
    printf("\nEnter roll to search: ");
    if (scanf("%d", &target) != 1) { printf("Invalid input\n"); clear_stdin(); return; }
    clear_stdin();
    FILE *fp = fopen(STUDENT_FILE, "r");
    if (!fp) { printf("%s not found.\n", STUDENT_FILE); return; }
    while (read_student_line(fp, &s)) {
        if (s.roll == target) { printf("\nStudent found:\nRoll: %d\nName: %s\nMarks: %.2f\n", s.roll, s.name, s.marks); found = 1; break; }
    }
    if (!found) printf("Student with roll %d not found.\n", target);
    fclose(fp);
}

void updateStudent(void) {
    int target; struct Student s; int found = 0;
    printf("\nEnter roll to update: ");
    if (scanf("%d", &target) != 1) { printf("Invalid input\n"); clear_stdin(); return; }
    clear_stdin();

    FILE *fp = fopen(STUDENT_FILE, "r");
    if (!fp) { printf("%s not found.\n", STUDENT_FILE); return; }

    if (make_backup(STUDENT_FILE) != 0) {
        printf("Warning: could not create backup. Aborting update.\n"); fclose(fp); return;
    }

    FILE *temp = fopen("temp_students.txt", "w");
    if (!temp) { printf("Could not open temporary file.\n"); fclose(fp); return; }

    while (read_student_line(fp, &s)) {
        if (s.roll == target) {
            found = 1;
            printf("Existing -> Roll: %d, Name: %s, Marks: %.2f\n", s.roll, s.name, s.marks);
            char namebuf[200]; float newmarks;
            printf("Enter new full name (leave blank to keep): ");
            if (!fgets(namebuf, sizeof(namebuf), stdin)) namebuf[0] = '\0';
            size_t ln = strlen(namebuf);
            if (ln && namebuf[ln-1] == '\n') namebuf[ln-1] = '\0';
            if (strlen(namebuf) != 0) { strncpy(s.name, namebuf, sizeof(s.name)-1); s.name[sizeof(s.name)-1] = '\0'; }
            printf("Enter new marks (or -1 to keep): ");
            if (scanf("%f", &newmarks) != 1) { clear_stdin(); printf("Invalid marks input; keeping old.\n"); }
            else { if (newmarks >= 0.0f) s.marks = newmarks; }
            clear_stdin();
            append_student_line(temp, &s);
        } else {
            append_student_line(temp, &s);
        }
    }

    fclose(fp);
    fclose(temp);

    if (found) {
        if (remove(STUDENT_FILE) != 0) printf("Error removing old file: %s\n", strerror(errno));
        else if (rename("temp_students.txt", STUDENT_FILE) != 0) printf("Error renaming temp file: %s\n", strerror(errno));
        else printf("Student updated successfully.\n");
    } else {
        remove("temp_students.txt"); printf("Student with roll %d not found.\n", target);
    }
}

void deleteStudent(void) {
    int target; struct Student s; int found = 0;
    printf("\nEnter roll to delete: ");
    if (scanf("%d", &target) != 1) { printf("Invalid input\n"); clear_stdin(); return; }
    clear_stdin();

    FILE *fp = fopen(STUDENT_FILE, "r");
    if (!fp) { printf("%s not found.\n", STUDENT_FILE); return; }

    if (make_backup(STUDENT_FILE) != 0) {
        printf("Warning: could not create backup. Aborting delete.\n"); fclose(fp); return;
    }

    FILE *temp = fopen("temp_students.txt", "w");
    if (!temp) { printf("Could not open temporary file.\n"); fclose(fp); return; }

    while (read_student_line(fp, &s)) {
        if (s.roll == target) { found = 1; /* skip */ }
        else append_student_line(temp, &s);
    }

    fclose(fp);
    fclose(temp);

    if (found) {
        if (remove(STUDENT_FILE) != 0) printf("Error removing old file: %s\n", strerror(errno));
        else if (rename("temp_students.txt", STUDENT_FILE) != 0) printf("Error renaming temp file: %s\n", strerror(errno));
        else printf("Student deleted successfully.\n");
    } else {
        remove("temp_students.txt"); printf("Student with roll %d not found.\n", target);
    }
}

/* ---------------- helpers ---------------- */
int read_student_line(FILE *fp, struct Student *s) {
    char line[LINE_BUF_SIZE];
    if (!fgets(line, sizeof(line), fp)) return 0;
    /* Try: roll "name with spaces" marks */
    int matched = sscanf(line, "%d \"%199[^\"]\" %f", &s->roll, s->name, &s->marks);
    if (matched == 3) return 1;
    /* Fallback: roll name marks (single-token name) */
    char name_small[200];
    matched = sscanf(line, "%d %199s %f", &s->roll, name_small, &s->marks);
    if (matched == 3) { strncpy(s->name, name_small, sizeof(s->name)-1); s->name[sizeof(s->name)-1] = '\0'; return 1; }
    return 0;
}

void append_student_line(FILE *fp, const struct Student *s) {
    /* replace any double quotes in name with apostrophes */
    char safe[sizeof(s->name)*2]; size_t si = 0;
    for (size_t i = 0; s->name[i] && si+1 < sizeof(safe); ++i) {
        safe[si++] = (s->name[i] == '"') ? '\'' : s->name[i];
    }
    safe[si] = '\0';
    fprintf(fp, "%d \"%s\" %.2f\n", s->roll, safe, s->marks);
}

int roll_exists(int roll) {
    struct Student s;
    FILE *fp = fopen(STUDENT_FILE, "r");
    if (!fp) return 0;
    int exists = 0;
    while (read_student_line(fp, &s)) {
        if (s.roll == roll) { exists = 1; break; }
    }
    fclose(fp);
    return exists;
}

int make_backup(const char *filename) {
    FILE *src = fopen(filename, "r");
    if (!src) return 0; /* nothing to back up */
#if defined(_WIN32) || defined(_WIN64)
    _mkdir(BACKUP_DIR);
#else
    mkdir(BACKUP_DIR, 0755);
#endif
    char backupname[256];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm); /* thread-safe variant where available */
    snprintf(backupname, sizeof(backupname), "%s/backup_%04d%02d%02d_%02d%02d%02d.bak",
             BACKUP_DIR,
             tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    FILE *dst = fopen(backupname, "w");
    if (!dst) { fclose(src); return -1; }
    char buf[4096]; size_t n;
    while ((n = fread(buf,1,sizeof(buf),src)) > 0) if (fwrite(buf,1,n,dst) != n) { fclose(src); fclose(dst); return -1; }
    fclose(src); fclose(dst);
    printf("Backup created: %s\n", backupname);
    return 0;
}

void clear_stdin(void) {
    int c;
    while ((c = getchar()) != EOF && c != '\n') {}
}
