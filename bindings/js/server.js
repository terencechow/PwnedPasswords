const express = require('express');
const path = require('path');
const fs = require('fs')

const pwnedPasswords = require('./build/Release/pwned_passwords.node');

const db = pwnedPasswords(.000001);

const dbFileName = path.join(__dirname, '../../data/pwned_passwords.db');
const textFileName = path.join(__dirname, '../../data/pwned-passwords-sample.txt');

if (fs.existsSync(dbFileName)) {
    db.initFromDb(dbFileName);
} else if (fs.existsSync(textFileName)) {
    db.initFromTextfile(textFileName, dbFileName);
} else {
    console.log("Could not find database file or text file to initialize from. Exiting...");
    process.exit(1);
}

const app = express();
app.use(express.json());

const port = 3000;

app.get('/', (req, res) => res.send('Hello World!'));

app.get('/check/:hash', (req, res) => {
    res.send(db.checkHash(req.params.hash.toUpperCase()));
})

app.post('/check/password', (req, res) => {
    res.send(db.checkPassword(req.body.password));
})

app.listen(port, () => console.log(`PwnedPassword Microaservice app listening on port ${port}!`))