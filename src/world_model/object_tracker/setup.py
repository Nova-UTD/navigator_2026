from setuptools import find_packages, setup

package_name = 'object_tracker'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Ragib Arnab',
    maintainer_email='ragib.arnab@gmail.com',
    description='Gives each detected object an ID and a velocity',
    license='MIT License',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'object_tracker = object_tracker.object_tracker:main'
        ],
    },
)
