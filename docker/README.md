## Why docker?
Docker is a runtime for stateless containers. That is to say: a Docker container will run the exact same software on my linux distribution as it will on your (e.g) Windows installation. All dependencies and setup operations are specified in what's called a 'Dockerfile', which makes it execute the exact same commands on irregardless of which pc you decide to run it on. This obviously has major benefits for reproduction of scientific results, and for not bloating your pc with libraries required by each and every tool you want to try. Hence its inclusion in this repository.

## Install instructions
I recommend running docker in rootless mode for security. Some operating systems like Fedora still require root access to set up the correct environment before intallation, so it would be best to ask your sysadmin for help. Most other operating systems allow any user to install docker in rootless mode. All appropriate instructions can be found [here](https://docs.docker.com/engine/security/rootless/).

## Building the image
Is as easy as:
```
cd your/lime/directory/docker
docker build -t lime .
```
Or if you decide to _not_ run docker rootless, add sudo before your docker commands.

## Running lime
```
docker run lime sh -c "lime --your --lime --commands"
```

If you need to recover data files from lime, run it with an additional `-v` flag like so:
```
docker run -v lime_output:/home/lime/output lime sh -c "lime --your --lime --commands"
```

Don't forget to specify the correct `-o` flag in your commands, e.g.:
```
docker run -v lime_output:/home/lime/output lime sh -c "lime generate -o /home/lime/output --some --more --flags"
```

Data can then be retrieved from `/var/lib/docker/volumes/lime_output/_data` or in rootless mode: `~/.local/share/docker/volumes/lime_output/_data`
